/*
 *  filterlistwindow.h
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

#ifndef FILTERLISTWINDOW_H
#define FILTERLISTWINDOW_H

#include <QDialog>
#include <QMainWindow>
#include <QAbstractItemModel>
#include <QEvent>
#include <QStatusBar>

#include "filter.h"

typedef QHash<QString, QString> FilterFieldMap;

class FilterModel;

class FilterListWindow : public QMainWindow
{
    Q_OBJECT

    public:
        FilterListWindow(QString filename, QWidget* parent=nullptr,
                         Qt::WindowFlags flags = Qt::WindowFlags());
        ~FilterListWindow();

    protected:
        QWidget* createTab(QString name);
        void setTabLabel(uint8_t type, int count);

    protected slots:
        void editItem(QModelIndex index);
        void tabButtonClicked(QAction* action);
        void createItem(uint8_t type);
        void deleteItem(uint8_t type);
        void load();
        void loadFile();
        void save();
        void saveAs();

    private:
        QString m_filename;
        QTabWidget* m_tabWidget;
        QStatusBar* m_statusBar;
        Filters* m_filters;
        FilterTypes* m_types;
        QHash<uint8_t, QTreeView*> m_views;
        QHash<uint8_t, FilterModel*> m_models;


};

class FilterModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
        FilterModel(Filters* filters, uint8_t type, QObject* parent=nullptr);
        virtual ~FilterModel();

        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex &index) const;
        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        virtual QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;

        void addFilter(QString filterPattern);
        void removeFilter(QModelIndex selection);

    private:
        Filters* m_filters;
        uint8_t m_type;
};


class FilterFormField : public QWidget
{
    Q_OBJECT

    public:
        FilterFormField(QString name, QString labeltext = QString(), QWidget* parent=nullptr);

        QString m_name;
        QString m_labeltext;
        QCheckBox* m_check;
        QLabel* m_label;
        QLineEdit* m_edit;

    public slots:
        bool eventFilter(QObject* object, QEvent* event);
        void stateChanged(int state);
};

//SubClassing QCheckBox so we can control the sequence of check/uncheck/partial when
//clicking "Toggle All"
class ToggleAllCheckBox : public QCheckBox
{
    protected:
        virtual void nextCheckState() override;

};



class FilterDialog : public QDialog
{
    Q_OBJECT

    public:

        static QString getFilter(QWidget* parent, const QString& title,
                const QString& filterString, bool* ok=nullptr,
                Qt::WindowFlags flags = Qt::WindowFlags(),
                Qt::InputMethodHints inputMethodHints = Qt::ImhNone);

    protected:
        FilterDialog(QWidget* parent=nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
        ~FilterDialog();

        void setData(const QString filterString);
        void createForm();

        QHash<QString, FilterFormField*> m_filterFields;
        ToggleAllCheckBox* m_toggleAll;

        QString m_spawnFilterString;
        FilterFieldMap m_spawnFilterMap;

        QString m_filterString;
        int m_fieldCount;
        int m_fieldsCheckedCount;
        bool m_hasTrailingColon;

    signals:
        void stateChanged(int state);

    protected slots:
        void resetForm();
        void acceptDialog();
        void fieldToggled(bool checked);
        void toggleAllToggled(int state);

};

// helper functions
void FilterString2FilterFieldMap(const QString filterString, FilterFieldMap* map);
QString FilterFieldMap2FilterString(FilterFieldMap* map);


#endif
