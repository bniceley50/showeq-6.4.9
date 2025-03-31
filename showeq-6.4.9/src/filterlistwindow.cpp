/*
 *  filterlistwindow.cpp
 *  Copyright 2024 by the respective ShowEQ Developers
 *
 *  This file is part of ShowEQ.
 *  http://www.sourceforge.net/projects/seq
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTreeView>
#include <QDebug>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QToolBar>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>

#include "diagnosticmessages.h"
#include "filtermgr.h"
#include "filterlistwindow.h"
#include "toolbaricons.h"



#define FILTERLISTCOLUMN_TABLE \
    X(FilterString)            \
    X(MinLevel)                \
    X(MaxLevel)

#define X(a) FLC_##a,
enum FilterListColumn {
    FILTERLISTCOLUMN_TABLE
    FLC_Max
};
#undef X

#define X(a) #a,
const QString FilterListColumnName[] = {
    FILTERLISTCOLUMN_TABLE
};
#undef X

enum DataItemUserRole
{
    DIUR_FilterStringWithLevelRange=Qt::UserRole + 1,
};


FilterListWindow::FilterListWindow(QString filename, QWidget* parent, Qt::WindowFlags flags):
    QMainWindow(parent, flags),
    m_filename(filename),
    m_tabWidget(nullptr),
    m_statusBar(nullptr),
    m_filters(nullptr),
    m_types(nullptr)
{
    //setModal(true);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(filename);

    m_types = new FilterTypes;
    uint8_t type;
    uint32_t mask;
    #define X(a, b, c) m_types->registerType(#a, type, mask);
    FILTER_TYPE_TABLE
    #undef X

    m_filters = new Filters(*m_types);

    QMenuBar* menubar = new QMenuBar(this);
    setMenuBar(menubar);

    QMenu* fileMenu = new QMenu("&File", this);

#if (QT_VERSION >= QT_VERSION_CHECK(6,3,0))
    fileMenu->addAction(ToolbarIcons::FileOpen(), "&Open", Qt::CTRL|Qt::Key_O, this, SLOT(load()));
    fileMenu->addAction(ToolbarIcons::FileSave(), "&Save", Qt::CTRL|Qt::Key_S, this, SLOT(save()));
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", Qt::CTRL|Qt::Key_W, this, SLOT(close()));
#else
    fileMenu->addAction(ToolbarIcons::FileOpen(), "&Open", this, SLOT(load()), Qt::CTRL|Qt::Key_O);
    fileMenu->addAction(ToolbarIcons::FileSave(), "&Save", this, SLOT(save()), Qt::CTRL|Qt::Key_S);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, SLOT(close()), Qt::CTRL|Qt::Key_W);
#endif

    menubar->addMenu(fileMenu);

    QToolBar* toolbar = new QToolBar(this);
    toolbar->addAction(ToolbarIcons::FileOpen(), "Open File", this, SLOT(load()));
    toolbar->addAction(ToolbarIcons::FileSave(), "Save File", this, SLOT(save()));

    addToolBar(toolbar);

    m_tabWidget = new QTabWidget(this);

    QStringList tabs = {
    #define X(a, b, c) #a,
        FILTER_TYPE_TABLE
    };
    #undef X

    for (uint8_t i=0; i < SIZEOF_FILTERS; ++i)
    {
        QWidget* t = createTab(m_types->name(i));
        m_tabWidget->addTab(t, "&" + m_types->name(i));
    }

    setCentralWidget(m_tabWidget);

    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);

    loadFile();

    show();

    m_statusBar->showMessage("Ready", 2000);

}

FilterListWindow::~FilterListWindow()
{
    if (m_filters)
        delete m_filters;

    if (m_types)
        delete m_types;

    qDeleteAll(m_models);
    m_models.clear();

    qDeleteAll(m_views);
    m_views.clear();
}

void FilterListWindow::setTabLabel(uint8_t type, int count)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    if (!m_tabWidget || !m_tabWidget->tabBar())
        return;

    m_tabWidget->tabBar()->setTabText(type, QString("&") + m_types->name(type)
#else
    if (!m_tabWidget)
        return;

    m_tabWidget->setTabText(type, QString("&") + m_types->name(type)
#endif
            + " (" + QString::number(count) + ")");
}

QWidget* FilterListWindow::createTab(QString name)
{
    uint8_t type = m_types->type(name);

    QWidget* w = new QWidget(this);
    w->setObjectName(name);
    QVBoxLayout* l = new QVBoxLayout(w);

    QToolBar* tabButtons = new QToolBar(w);

    QAction* tmpAction = new QAction("+", nullptr);
    tmpAction->setToolTip("Add New Item");
    tmpAction->setProperty("type", m_types->type(name));
    tmpAction->setProperty("action", "add");
    tabButtons->addAction(tmpAction);

    tmpAction = new QAction("-", nullptr);
    tmpAction->setToolTip("Delete Selected Item");
    tmpAction->setProperty("type", m_types->type(name));
    tmpAction->setProperty("action", "delete");
    tabButtons->addAction(tmpAction);

    l->addWidget(tabButtons);
    connect(tabButtons, SIGNAL(actionTriggered(QAction*)), this, SLOT(tabButtonClicked(QAction*)));


    QTreeView* t = new QTreeView();
    m_views[type] = t;
    t->setRootIsDecorated(false);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->expandAll();
    t->setItemsExpandable(false);

    connect(t, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editItem(QModelIndex)));
    l->addWidget(t);

    return w;
}

void FilterListWindow::tabButtonClicked(QAction* action)
{
    uint8_t type = action->property("type").toInt();
    QString a = action->property("action").toString();
    if (a == "add")
        createItem(type);
    else if (a == "delete")
        deleteItem(type);
}

void FilterListWindow::createItem(uint8_t type)
{
    bool ok = false;
    QString filterString;
    filterString = FilterDialog::getFilter(this, "Add " + m_types->name(type) + " Filter", filterString, &ok);
    if (ok)
    {
        m_models[type]->addFilter(filterString);
        setTabLabel(type, m_models[type]->rowCount());
    }
}

void FilterListWindow::deleteItem(uint8_t type)
{
    QModelIndexList selectedRows = m_views[type]->selectionModel()->selectedRows();
    if (selectedRows.size())
    {
        QModelIndex selected = selectedRows[0]; //only one selection allowed, so we can take the first
        m_models[type]->removeFilter(selected);
        setTabLabel(type, m_models[type]->rowCount());
    }
}

void FilterListWindow::editItem(QModelIndex index)
{
    //TODO rework the whole filterdialog/filtermodel/filters call chain
    //to to better handle min/max level rather than this kludge
    //
    const QAbstractItemModel* model = index.model();
    QString filterString = model->data(index, DIUR_FilterStringWithLevelRange).toString();
    uint8_t type = index.internalId();
    bool ok = false;
    filterString = FilterDialog::getFilter(this, "Edit " + m_types->name(type) + " Filter", filterString, &ok);
    if (ok)
    {
        m_models[type]->removeFilter(index);
        m_models[type]->addFilter(filterString);
    }
}

void FilterListWindow::load()
{
    QString fn = QFileDialog::getOpenFileName(this, "Open File",
            m_filename, "XML Files (*.xml)");
    if (fn.isEmpty())
    {
        m_statusBar->showMessage("File Open Cancelled", 2000);
        return;
    }

    m_filename = fn;
    setWindowTitle(m_filename);
    loadFile();
}

void FilterListWindow::loadFile()
{

    for (uint8_t i=0; i < SIZEOF_FILTERS; ++i)
        m_views[i]->setModel(nullptr);

    qDeleteAll(m_models);
    m_models.clear();

    m_filters->load(m_filename);

    for (int i=0; i < SIZEOF_FILTERS; ++i)
    {
        m_models[i] = new FilterModel(m_filters, i);
        m_views[i]->setModel(m_models[i]);
        setTabLabel(i, m_models[i]->rowCount());

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
        m_views[i]->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
        m_views[i]->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    }

    m_statusBar->showMessage(QString("Loaded %1").arg(m_filename), 2000);
}

void FilterListWindow::save()
{
    if (m_filename.isEmpty())
        saveAs();
    else
        if (!m_filters->save())
            m_statusBar->showMessage(QString("Could not write to %1").arg(m_filename),
                    2000);
        else
            m_statusBar->showMessage(QString("Saved %1").arg(m_filename), 2000);
}

void FilterListWindow::saveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save File",
            m_filename, "XML Files (*.xml)");
    if (fn.isEmpty())
    {
        m_statusBar->showMessage("Save File Cancelled", 2000);
        return;
    }

    m_filename = fn;
    setWindowTitle(m_filename);
    save();
}


FilterModel::FilterModel(Filters* filters, uint8_t type, QObject* parent) :
    QAbstractItemModel(parent),
    m_filters(filters),
    m_type(type)
{ }

FilterModel::~FilterModel()
{}

QModelIndex FilterModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column, m_type);
}

QModelIndex FilterModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int FilterModel::rowCount(const QModelIndex &parent) const
{
    return m_filters->numFilters(m_type);
}

int FilterModel::columnCount(const QModelIndex &parent) const
{
    return FLC_Max;
}

QVariant FilterModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
            return FilterListColumnName[section];

        default:
            return QVariant();
    }
}

QVariant FilterModel::data(const QModelIndex &index, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
            switch(index.column())
            {
                case FLC_FilterString:
                    return m_filters->getOrigFilterString(m_type, index.row());
                case FLC_MinLevel:
                    return m_filters->getMinLevel(m_type, index.row());
                case FLC_MaxLevel:
                    return m_filters->getMaxLevel(m_type, index.row());
                default:
                    return QVariant();
            }
        case DIUR_FilterStringWithLevelRange:
            {
                int minLevel = m_filters->getMinLevel(m_type, index.row());
                int maxLevel = m_filters->getMaxLevel(m_type, index.row());
                QString filterString = m_filters->getOrigFilterString(m_type, index.row());
                if (minLevel > 0 || maxLevel > 0)
                {
                    filterString += ';';
                    if (minLevel > 0)
                    {
                        filterString += QString::number(minLevel);
                        if (maxLevel > 0)
                            filterString += "-" + QString::number(maxLevel);
                        else
                            filterString += "-" + QString::number(SHRT_MAX);
                    }
                    else
                    {
                        filterString += "0-" + QString::number(maxLevel);
                    }
                }
                return filterString;
            }

        default:
            return QVariant();
    }
}

void FilterModel::addFilter(QString filterPattern)
{
    //TODO rework the whole filterdialog/filtermodel/filters call chain
    //to to better handle min/max level rather than this kludge
    int minLevel = 0;
    int maxLevel = 0;

    QString workString = filterPattern;
    int breakpoint = workString.indexOf(';');
    if (breakpoint == -1)
    {
        beginInsertRows(QModelIndex(), rowCount(), 1);
        m_filters->addFilter(m_type, filterPattern);
        endInsertRows();
    }
    else
    {
        //this is basically a copy of the level string parsing code in FilterItem()
        filterPattern = workString.left(breakpoint);
        QString levelString = workString.mid(breakpoint+1);
        breakpoint = levelString.indexOf('-');
        bool ok;
        int level;
        if (breakpoint == -1)
        {
            level = levelString.toInt(&ok);
            if (ok)
                minLevel = level;
        }
        else
        {
            level = levelString.left(breakpoint).toInt(&ok);
            if (ok)
                minLevel = level;

            levelString = levelString.mid(breakpoint+1);
            if (levelString.isEmpty())
            {
                maxLevel = SHRT_MAX;
            }
            else
            {
                level = levelString.toInt(&ok);
                if (ok)
                    maxLevel = level;
            }
        }
        if (maxLevel < minLevel)
            maxLevel = minLevel;


        beginInsertRows(QModelIndex(), rowCount(), 1);
        m_filters->addFilter(m_type, filterPattern, minLevel, maxLevel);
        endInsertRows();


    }

    emit dataChanged(index(rowCount()-1, 0), index(rowCount(), 1));
}

void FilterModel::removeFilter(QModelIndex selection)
{
    beginRemoveRows(QModelIndex(), selection.row(), 1);
    m_filters->remFilter(m_type, m_filters->getFilterString(m_type, selection.row()));
    endRemoveRows();
}


FilterFormField::FilterFormField(QString name, QString labeltext, QWidget* parent) :
    QWidget(parent),
    m_name(name),
    m_labeltext(labeltext),
    m_check(nullptr),
    m_label(nullptr),
    m_edit(nullptr)
{
    if (m_labeltext.isNull())
        m_labeltext = m_name;

    m_check = new QCheckBox(this);
    m_check->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    m_label = new QLabel(m_labeltext, this);
    QSizePolicy tmpPolicy = m_label->sizePolicy();
    tmpPolicy.setVerticalPolicy(QSizePolicy::Fixed);
    m_label->setSizePolicy(tmpPolicy);

    m_edit = new QLineEdit(this);
    tmpPolicy = m_edit->sizePolicy();
    tmpPolicy.setVerticalPolicy(QSizePolicy::Fixed);
    m_edit->setSizePolicy(tmpPolicy);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(m_check);
    layout->addWidget(m_label);
    layout->addWidget(m_edit);

    connect(m_check, SIGNAL(toggled(bool)), m_edit, SLOT(setEnabled(bool)));
    m_edit->installEventFilter(this);
}

bool FilterFormField::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_edit && event->type() == QEvent::MouseButtonPress)
    {
        m_check->setChecked(true);
        //stateChanged(Qt::Checked);
        m_edit->setFocus(Qt::MouseFocusReason);
        return true;
    }
    return false;
}

void FilterFormField::stateChanged(int state)
{
    bool old_check_state = m_check->blockSignals(true);
    bool old_edit_state = m_edit->blockSignals(true);
    switch (state)
    {
        case Qt::Unchecked:
            m_check->setChecked(false);
            m_edit->setEnabled(false);
            break;
        case Qt::PartiallyChecked:
            break;
        case Qt::Checked:
            m_check->setChecked(true);
            m_edit->setEnabled(true);
            break;
    }
    m_check->blockSignals(old_check_state);
    m_edit->blockSignals(old_edit_state);
}

void ToggleAllCheckBox::nextCheckState()
{
    switch(checkState())
    {
        case Qt::Unchecked:
            setCheckState(Qt::Checked);
            break;
        case Qt::PartiallyChecked:
            setCheckState(Qt::Unchecked);
            break;
        case Qt::Checked:
            setCheckState(Qt::Unchecked);
            break;
    }

}

FilterDialog::FilterDialog(QWidget* parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    m_toggleAll(nullptr),
    m_filterString(QString()),
    m_fieldCount(0),
    m_fieldsCheckedCount(0)
{
    //init m_spawnFilterMap
    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        if (field == FSF_Info) continue;

        QString name = FilterStringFieldName[field];
        m_spawnFilterMap[name] = "";
    }
    //starting with Head since Light is handled above
    for (int field = FSIF_Head; field < FSIF_Max; ++field)
    {
        QString name = FilterStringInfoFieldName[field];
        m_spawnFilterMap[name] = "";
    }
    m_spawnFilterMap[FSF_MINLEVEL_NAME] = "";
    m_spawnFilterMap[FSF_MAXLEVEL_NAME] = "";

    createForm();
}

FilterDialog::~FilterDialog()
{ }

void FilterDialog::createForm()
{

    const int colspc_x = 20;
    const int colspc_y = 1;

    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();

    // info/instructions
    QLabel* tmpLabel = new QLabel("All fields except '" FSF_MINLEVEL_LABEL "' and '" FSF_MAXLEVEL_LABEL "' accept Regular Expression syntax.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);
    tmpLabel = new QLabel("For an exact level match or matching multiple levels using a RegEx, use the 'Level' field.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);
    tmpLabel = new QLabel("To limit to a simple level range (no RegEx), use the '" FSF_MINLEVEL_LABEL "' and '" FSF_MAXLEVEL_LABEL "' fields.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);
    tmpLabel = new QLabel("Any fields left blank or not checked will not be matched against.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);

    pageLayout->addItem(new QSpacerItem(colspc_x, 25));

    //using an extra widget so the spacing lines up
    QWidget* toggleAllWidget = new QWidget();
    QHBoxLayout* toggleAllLayout = new QHBoxLayout(toggleAllWidget);
    m_toggleAll = new ToggleAllCheckBox();
    m_toggleAll->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_toggleAll->setTristate(true);
    toggleAllLayout->addWidget(m_toggleAll);
    toggleAllLayout->addWidget(new QLabel("Toggle All", this));
    connect(m_toggleAll, SIGNAL(stateChanged(int)), this, SLOT(toggleAllToggled(int)));

    pageLayout->addLayout(gridLayout);
    gridLayout->addWidget(toggleAllWidget, 0, 0);

    #define X(a, b) #a,
    const QString labels[] = {
        FILTERSTRINGFIELD_TABLE
    };
    #undef X

    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        if (field == FSF_Info) continue;

        QString name = FilterStringFieldName[field];
        QString label = labels[field];
        m_filterFields[name] = new FilterFormField(name, label);
        m_fieldCount++;

    }

    #define X(a, b) #a,
    const QString info_labels[] = {
        FILTERSTRINGINFOFIELD_TABLE
    };
    #undef X

    // Starting with head, since Light is already created above.
    for (int field = FSIF_Head; field < FSIF_Max; ++field)
    {
        QString name = FilterStringInfoFieldName[field];
        QString label = info_labels[field];
        m_filterFields[name] = new FilterFormField(name, label);
        m_fieldCount++;
    }

    //not part of normal regex string, but still part of filter
    m_filterFields[FSF_MINLEVEL_NAME] = new FilterFormField(FSF_MINLEVEL_NAME, FSF_MINLEVEL_LABEL);
    m_filterFields[FSF_MAXLEVEL_NAME] = new FilterFormField(FSF_MAXLEVEL_NAME, FSF_MAXLEVEL_LABEL);
    m_fieldCount += 2;

    const QString formFieldOrder[] = { "Name", "LastName", "Guild", "Race", "Class",
        "Deity", "Level", FSF_MINLEVEL_NAME, FSF_MAXLEVEL_NAME, "X", "Y", "Z", "NPC", "Type",
        "GM", "RTeam", "DTeam", "Spawn", "Light",
        //Info fields
        "H", "C", "A", "W", "G", "L", "F", "1", "2" };

    int row = 1; //toggle all is row 0
    int col = 0;
    for (auto fieldname : formFieldOrder)
    {
        gridLayout->addWidget(m_filterFields[fieldname], row, col++);

        connect(m_toggleAll, SIGNAL(stateChanged(int)), m_filterFields[fieldname], SLOT(stateChanged(int)));
        connect(m_filterFields[fieldname]->m_check, SIGNAL(toggled(bool)), this, SLOT(fieldToggled(bool)));

        if (fieldname == "Guild" || fieldname == "Deity" ||
                fieldname == FSF_MAXLEVEL_NAME || fieldname == "Z" ||
                fieldname == "GM" || fieldname == "Spawn" ||
                fieldname == "Light" ||
                fieldname == "A" || fieldname == "L")
        {
            row++;
            col = 0;
        }
    }

    //buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    pageLayout->addItem(new QSpacerItem(colspc_x, 25));
    pageLayout->addLayout(buttonLayout);

    QPushButton* resetButton = new QPushButton("Reset");
    resetButton->setDefault(false);
    resetButton->setAutoDefault(false);
    buttonLayout->addWidget(resetButton);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(resetForm()));

    buttonLayout->addItem(new QSpacerItem(colspc_x, colspc_y));

    QPushButton* okButton = new QPushButton("Ok");
    okButton->setDefault(false);
    okButton->setAutoDefault(false);
    buttonLayout->addWidget(okButton);
    connect(okButton, SIGNAL(clicked()), this, SLOT(acceptDialog()));

    QPushButton* cancelButton = new QPushButton("Cancel");
    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    buttonLayout->addWidget(cancelButton);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void FilterDialog::setData(const QString filterString)
{
    m_spawnFilterString = filterString;

    FilterString2FilterFieldMap(filterString, &m_spawnFilterMap);

    resetForm();
}

void FilterDialog::resetForm()
{
    m_fieldsCheckedCount = 0;
    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        if (field == FSF_Info) continue;

        QString name = FilterStringFieldName[field];

        m_filterFields[name]->m_edit->setText(m_spawnFilterMap[name]);

        if (m_filterFields[name]->m_edit->text().length())
        {
            m_filterFields[name]->stateChanged(Qt::Checked);
            m_fieldsCheckedCount++;
        }
        else
        {
            m_filterFields[name]->stateChanged(Qt::Unchecked);
        }
    }

    //starting with Head, since Light was handled above
    for (int field = FSIF_Head; field < FSIF_Max; ++field)
    {
        QString name = FilterStringInfoFieldName[field];
        m_filterFields[name]->m_edit->setText(m_spawnFilterMap[name]);

        if (m_filterFields[name]->m_edit->text().length())
        {
            m_filterFields[name]->stateChanged(Qt::Checked);
            m_fieldsCheckedCount++;
        }
        else
        {
            m_filterFields[name]->stateChanged(Qt::Unchecked);
        }
    }

    //not part of normal regex string, but still part of filter
    m_filterFields[FSF_MINLEVEL_NAME]->m_edit->setText(m_spawnFilterMap[FSF_MINLEVEL_NAME]);
    if (m_filterFields[FSF_MINLEVEL_NAME]->m_edit->text().length())
    {
        m_filterFields[FSF_MINLEVEL_NAME]->stateChanged(Qt::Checked);
        m_fieldsCheckedCount++;
    }
    else
    {
        m_filterFields[FSF_MINLEVEL_NAME]->stateChanged(Qt::Unchecked);
    }
    m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->setText(m_spawnFilterMap[FSF_MAXLEVEL_NAME]);
    if (m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->text().length())
    {
        m_filterFields[FSF_MAXLEVEL_NAME]->stateChanged(Qt::Checked);
        m_fieldsCheckedCount++;
    }
    else
    {
        m_filterFields[FSF_MAXLEVEL_NAME]->stateChanged(Qt::Unchecked);
    }

    bool old_state = m_toggleAll->blockSignals(true);
    if (m_fieldsCheckedCount == 0)
        m_toggleAll->setCheckState(Qt::Unchecked);
    else if (m_fieldsCheckedCount == m_fieldCount)
        m_toggleAll->setCheckState(Qt::Checked);
    else
        m_toggleAll->setCheckState(Qt::PartiallyChecked);
    m_toggleAll->blockSignals(old_state);

}

void FilterDialog::acceptDialog()
{
    FilterFieldMap map;

    //if enabled, add to map
    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        if (field == FSF_Info) continue;

        QString name = FilterStringFieldName[field];
        if (m_filterFields[name]->m_edit->isEnabled())
            map[name] = m_filterFields[name]->m_edit->text();
    }
    //starting with Head since Light is handled above
    for (int field = FSIF_Head; field < FSIF_Max; ++field)
    {
        QString name = FilterStringInfoFieldName[field];
        if (m_filterFields[name]->m_edit->isEnabled())
            map[name] = m_filterFields[name]->m_edit->text();
    }
    //not part of normal regex string, but still part of filter
    if (m_filterFields[FSF_MINLEVEL_NAME]->m_edit->isEnabled())
        map[FSF_MINLEVEL_NAME] = m_filterFields[FSF_MINLEVEL_NAME]->m_edit->text();

    if (m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->isEnabled())
        map[FSF_MAXLEVEL_NAME] = m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->text();


    m_filterString = FilterFieldMap2FilterString(&map);

    done(QDialog::Accepted);
}

void FilterDialog::fieldToggled(bool checked)
{
    if (checked)
        m_fieldsCheckedCount++;
    else
        m_fieldsCheckedCount--;

    bool old_state = m_toggleAll->blockSignals(true);
    if (m_fieldsCheckedCount == m_fieldCount)
        m_toggleAll->setCheckState(Qt::Checked);
    else if (m_fieldsCheckedCount == 0)
        m_toggleAll->setCheckState(Qt::Unchecked);
    else
        m_toggleAll->setCheckState(Qt::PartiallyChecked);
    m_toggleAll->blockSignals(old_state);
}

void FilterDialog::toggleAllToggled(int state)
{
    if (sender() != m_toggleAll)
        return;

    switch(state)
    {
        case Qt::Checked:
            m_fieldsCheckedCount = m_fieldCount;
            break;
        case Qt::Unchecked:
            m_fieldsCheckedCount = 0;
            break;
        case Qt::PartiallyChecked:
            m_toggleAll->setCheckState(Qt::Checked);
            m_fieldsCheckedCount = m_fieldCount;
    }
    emit stateChanged(m_toggleAll->checkState());
}

QString FilterDialog::getFilter(QWidget* parent, const QString& title,
        const QString& filterString, bool* ok, Qt::WindowFlags flags,
        Qt::InputMethodHints inputMethodHints)
{
    FilterDialog* dlg = new FilterDialog(parent, flags);
    dlg->setWindowTitle(title);
    dlg->setData(filterString);

    const int ret = dlg->exec();
    if (ok)
        *ok = ret;

    QString result;
    if (ok)
        result = dlg->m_filterString;

    dlg->deleteLater();
    return result;
}


void FilterString2FilterFieldMap(const QString filterString, FilterFieldMap* map)
{
    if (!map || !filterString.length())
        return;

    QString levelSuffix;
    QString regex;
    int minLevel = -1;
    int maxLevel = -1;

    int split = filterString.lastIndexOf(';');
    if (split == -1)
    {
        regex = filterString;
    }
    else
    {
        regex = filterString.left(split);
        levelSuffix = filterString.mid(split+1);
    }


    // parse level range string
    if (levelSuffix.length())
    {
        auto range = levelSuffix.split('-');
        bool ok = false;

        if (range.size() == 1)
        {
            //no dash, only a single level specified - treat as exact match
            int level = range[0].toInt(&ok);
            if (ok)
            {
                minLevel = level;
                maxLevel = level;
            }
            else
            {
                seqWarn("Could not parse level: %s", range[0].toLatin1().data());
            }
        }
        else if (range.size() == 2)
        {
            //one dash, two fields - treat as range
            int level = range[0].toInt(&ok);
            if (ok)
                minLevel = level;
            else
                seqWarn("Could not parse min level: %s", range[0].toLatin1().data());

            ok = false;
            level = range[1].toInt(&ok);
            if (ok)
                maxLevel = level;
            else
                seqWarn("Could not parse max level: %s", range[0].toLatin1().data());


            // if range wasn't fully/correctly specified, use defaults
            minLevel = (minLevel > -1) ? minLevel : 0;
            maxLevel = (maxLevel > -1) ? maxLevel : SHRT_MAX;

        }
        else
        {
            seqWarn("Ignoring malformed level range string.");
        }

        if (maxLevel < minLevel)
        {
            int tmp = maxLevel;
            maxLevel = minLevel;
            minLevel = tmp;
        }
    }


    //process filter string and set form/map fields
    QStringList tokens = regex.split(":");

    QStringList::const_iterator itr = tokens.begin();
    for (; itr < tokens.end(); ++itr)
    {
        QString name = *itr;
        if (!map->contains(name))
        {
            if (!name.length() && itr == tokens.end() - 1)
            {
                //filter string has an ending : that we can ignore
                continue;
            }

            // Info isn't in the map, but we need to process it.
            // Also, if there are multi-field wildcards, it could parse
            // as a name of ".*"
            // Otherwise, skip any unknown fields
            if (name != "Info" && name != ".*")
            {
                seqWarn("Ignoring unknown filter string field: %s", name.toLatin1().data());
                ++itr; // skip this field's data
                continue;
            }
        }

        // handle multi-field wildcards
        if (name == ".*")
        {
            if (++itr == tokens.end())
                break;

            QString value = *itr;
            if (map->contains(value) || value == "Info")
            {
                // value is the next specified field name, so we'll back the
                // iterator back to 'name == ".*"' and restart the loop
                --itr;
                continue;
            }
            else
            {
                // we have a value but we don't know what field it belongs to.
                // If the next field is set, we could figure it out by working
                // backwards, but it's probably not worth the effort.
                // So we're just going to warn and ignore it
                seqWarn("A match value of \"%s\" was found, but no field was specified. Ignoring.",
                        value.toLatin1().data());
                continue;
            }
        }

        // get field data
        if (++itr == tokens.end())
            break;
        QString value = *itr;
        if (!value.trimmed().length())
            continue;

        if (name == "Name" && (value == "Door" || value == "Drop"))
        {
            //we add a colon to door and drop names, so it makes
            //parsing a little more complicated.
            value += ":";
            if (++itr == tokens.end())
                break;
            value += *itr;
            if (value.trimmed().length() <= 1)
                continue;
        }

        if (name == "Info")
        {
            //Info field contains space-separated slot:item pairs, and the
            //items themselves can also contain spaces.  So special parsing
            //is needed.
            bool info_done = false;
            QString subfield_name = value;
            while (itr != tokens.end() && !info_done)
            {
                //strip multi field wildcards from name (note, order matters here)
                subfield_name = subfield_name.remove("( | .* )");
                subfield_name = subfield_name.remove(".*");

                // Check the name against valid sub-fields, because we could
                // be past the Info field and into the next main field
                bool is_info_field = false;
                for (int field = FSIF_Light; field < FSIF_Max; ++field)
                {
                    if (subfield_name == FilterStringInfoFieldName[field])
                    {
                        is_info_field = true;
                        break;
                    }
                }

                if (!is_info_field)
                {
                    info_done = true;
                    continue;
                }

                // get value
                if (++itr == tokens.end())
                    break;
                value = *itr;
                if (!value.trimmed().length())
                    continue;

                //replace multi field wildcards in value/next
                value = value.replace("( | .* )", " ");

                int delim = value.lastIndexOf(' ');
                QString next_subfield_name = value.mid(delim+1);
                value = value.left(delim);

                (*map)[subfield_name] = value.trimmed();

                subfield_name = next_subfield_name;

            }

            if (itr == tokens.end())
                break;
        }
        else
        {
            (*map)[name] = value.trimmed();
        }
    }

    if (minLevel > -1)
        (*map)[FSF_MINLEVEL_NAME] = QString::number(minLevel);

    if (maxLevel > -1)
        (*map)[FSF_MAXLEVEL_NAME] = QString::number(maxLevel);

}

QString FilterFieldMap2FilterString(FilterFieldMap* map)
{
    if (!map)
        return QString();

    QString filterString;
    bool wildcard = false;
    bool has_first_match = false;

    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        QString name = FilterStringFieldName[field];

        if (name == "Info")
        {
            //info subfields need special handling
            bool info_added = false;
            bool info_wildcard = false;
            for (int info_field = FSIF_Light; info_field < FSIF_Max; ++info_field)
            {
                QString subfield_name = FilterStringInfoFieldName[info_field];
                if (!map->contains(subfield_name) || !(*map)[subfield_name].trimmed().length())
                {
                    if (!info_wildcard)
                    {
                        info_wildcard = true;
                    }
                    continue;
                }

                QString value = (*map)[subfield_name];
                value = value.trimmed();

                if (!info_added)
                {
                    if (wildcard)
                    {
                        wildcard = false;
                        filterString += ".*:Info:";
                    }
                    else
                    {
                        filterString += "Info:";
                    }
                    info_added = true;
                }

                if (info_wildcard)
                {
                    info_wildcard = false;
                    // we need to handle 2 cases here
                    // 1. match-field ignore-field match-field
                    // 2. match-field matchfield
                    // If we naively insert .* like we do elsewhere, we'll
                    // wind up with " .* " which will never match case 2.
                    // But we also don't want to just not include spaces
                    // in the match, because we don't want to accidentally
                    // match a different field/value (especially with short
                    // field names like C or A.
                    if (filterString.length() && filterString.endsWith(" "))
                    {
                        filterString.chop(1);
                        filterString += "( | .* )";
                    }
                    else
                    {
                        filterString += ".*";
                    }
                }

                filterString += subfield_name;
                filterString += ":";
                filterString += value;
                filterString += " ";
            }
            //end of Info loop, tidy up
            if (info_added)
            {
                if (info_wildcard)
                {
                    info_wildcard = false;
                    filterString += ".*:";
                }
                else
                {
                    filterString += ":";
                }
            }
        }
        else
        {
            if (!map->contains(name) || !(*map)[name].trimmed().length())
            {
                if (has_first_match && !wildcard)
                {
                    wildcard = true;
                }
                continue;
            }

            QString value = (*map)[name];
            value = value.trimmed();

            has_first_match = true;

            if (wildcard)
            {
                wildcard = false;
                filterString += ".*:";
            }
            filterString += name;
            filterString += ":";

            //Remove/change :'s depending on the field
            if (name == "Spawn")
                filterString += value.replace(':', '.');
            else if (name != "Name")
                filterString += value.remove(':');
            else
                filterString += value;

            filterString += ":";

        }
    }

    //min/max level are not part of normal regex string, but still part of filter
    int minLevel = -1;
    int maxLevel = -1;

    if (map->contains(FSF_MINLEVEL_NAME))
    {
        QString value = (*map)[FSF_MINLEVEL_NAME];
        value = value.trimmed();
        bool ok = false;
        int level = value.toInt(&ok);
        if (ok)
            minLevel = level;
    }

    if (map->contains(FSF_MAXLEVEL_NAME))
    {
        QString value = (*map)[FSF_MAXLEVEL_NAME];
        value = value.trimmed();
        bool ok = false;
        int level = value.toInt(&ok);
        if (ok)
            maxLevel = level;
    }

    if (minLevel >= 0 || maxLevel >= 0)
    {
        minLevel = (minLevel >= 0) ? minLevel : 0;
        maxLevel = (maxLevel >= 0) ? maxLevel : SHRT_MAX;

        if (maxLevel < minLevel)
        {
            int tmp = maxLevel;
            maxLevel = minLevel;
            minLevel = tmp;
        }

        filterString += ";";
        filterString += QString::number(minLevel);
        filterString += "-";
        filterString += QString::number(maxLevel);
    }

    return filterString;
}


#ifndef QMAKEBUILD
#include "filterlistwindow.moc"
#endif
