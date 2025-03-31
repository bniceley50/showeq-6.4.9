/*
 *  seqlistview.cpp
 *  Copyright 2001,2007 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2005-2007, 2019 by the respective ShowEQ Developers
 *
 *  Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net) 
 *  for use under the terms of the GNU General Public License, 
 *  incorporated herein by reference.
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

#include <QHeaderView>
#include <QSizePolicy>
#include <QtGlobal>

#include "seqlistview.h"
#include "main.h"

SEQListView::SEQListView(const QString prefName,
                         QWidget* parent,
                         const char* name,
                         Qt::WindowFlags f)
    : QTreeWidget(parent),
    m_preferenceName(prefName),
    m_sortColumn(0),
    m_sortIncreasing(true)
{
    setObjectName(name);
    setWindowFlags(f);

    // setup common listview defaults
    setSortingEnabled(true);
    setSorting(0, true);
    setRootIsDecorated(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    header()->setFirstSectionMovable(true);
    connect(header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(setSorting(int, Qt::SortOrder)));
#endif

}

SEQListView::~SEQListView()
{
}

const QString& SEQListView::columnPreferenceName(int column) const
{
    // return the base name of the preference for the requested column
    return m_columns[column];
}

QSizePolicy SEQListView::sizePolicy() const
{
    return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

int SEQListView::addColumn(const QString& label,
        int width)
{
    return addColumn(label, label, width);
}

int SEQListView::addColumn(const QString& label,
        const QString& preference,
        int width)
{
    // add the column to the list of preferences
    m_columns.append(preference);

    // add the column to the list of headers
    m_headers.append(label);

    // update the header row
    setHeaderLabels(m_headers);

    // return the column index
    return m_headers.indexOf(label);
}

void SEQListView::removeColumn(int column)
{
    // remove the preference from the list
    m_columns.removeAt(column);

    // remove the header from the list
    m_headers.removeAt(column);

    //update the header row
    setHeaderLabels(m_headers);
}

void SEQListView::insertItem(QTreeWidgetItem* item) {
    insertTopLevelItem(topLevelItemCount(), item);
}

void SEQListView::setSorting(int column, bool increasing)
{
    // save the sort information
    m_sortColumn = column;
    m_sortIncreasing = increasing;

    // set the sort order in the underlying listview
    sortByColumn(column, increasing ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void SEQListView::setSorting(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortIncreasing = (order == Qt::AscendingOrder) ? true : false;
    sortByColumn(column, order);
}

void SEQListView::savePrefs()
{
    // only save the preferences if visible
    if (isVisible())
    {
        int i;
        int width;
        QString columnName;
        QString show = "Show";

        // save the column width's/visibility
        for (i = 0; i < columnCount(); i++)
        {
            columnName = columnPreferenceName(i);
            width = columnWidth(i);
            if (!header()->isSectionHidden(i) && width != 0)
            {
                pSEQPrefs->setPrefInt(columnName + "Width", preferenceName(), width);
                pSEQPrefs->setPrefBool(show + columnName, preferenceName(), true);
            }
            else
                pSEQPrefs->setPrefBool(show + columnName, preferenceName(), false);
        }

        // save the column order
        QString tempStr, tempStr2;
        if (header()->count() > 0)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
            tempStr = QString::asprintf("%d", header()->logicalIndex(0));
#else
            tempStr.sprintf("%d", header()->logicalIndex(0));
#endif
        for(i=1; i < header()->count(); i++)
        {
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
            tempStr2 = QString::asprintf(":%d", header()->logicalIndex(i));
#else
            tempStr2.sprintf(":%d", header()->logicalIndex(i));
#endif
            tempStr += tempStr2;
        }
        pSEQPrefs->setPrefString("ColumnOrder", preferenceName(), tempStr);

        // save the current sorting state
        pSEQPrefs->setPrefInt("SortColumn", preferenceName(), m_sortColumn);
        pSEQPrefs->setPrefBool("SortIncreasing", preferenceName(),
                m_sortIncreasing);
    }
}

void SEQListView::restoreColumns()
{
    int i;
    int width;
    QString columnName;
    QString show = "Show";

    // restore the column width's/visibility
    for (i = 0; i < columnCount(); i++)
    {
        columnName = columnPreferenceName(i);

        // check if the column is visible
        if (pSEQPrefs->getPrefBool(show + columnName, preferenceName(), true))
        {
            // check if the column has a width specified
            if (pSEQPrefs->isPreference(columnName + "Width", preferenceName()))
            {
                // use the specified column width
                width = pSEQPrefs->getPrefInt(columnName + "Width", preferenceName());
            }
            else
            {
                width = header()->sectionSizeHint(i);
            }

#if QT_VERSION >= 0x050000
            header()->setSectionResizeMode(i, QHeaderView::Interactive);
#else
            header()->setResizeMode(i, QHeaderView::Interactive);
#endif
            header()->resizeSection(i, width);
            setColumnWidth(i, width);
            header()->setSectionHidden(i, false);
        }
        else
        {
            // column is not visible, hide it.
#if QT_VERSION >= 0x050000
            header()->setSectionResizeMode(i, QHeaderView::Interactive);
#else
            header()->setResizeMode(i, QHeaderView::Interactive);
#endif
            header()->resizeSection(i, 0);
            setColumnWidth(i, 0);
            header()->setSectionHidden(i, true);
        }
    }

    // restore the column order
    QString tStr = pSEQPrefs->getPrefString("ColumnOrder", preferenceName(), "N/A");

    QHeaderView* hdr = header();
    if (tStr != "N/A")
    {
        int i = 0;
        while (!tStr.isEmpty())
        {
            int toIndex;
            if (tStr.indexOf(':') != -1)
            {
                toIndex = tStr.left(tStr.indexOf(':')).toInt();
                tStr = tStr.right(tStr.length() - tStr.indexOf(':') - 1);
            }
            else
            {
                toIndex = tStr.toInt();
                tStr = "";
            }
            hdr->moveSection(hdr->visualIndex(toIndex), i++);
        }
    }

    // restore sorting state
    setSorting(pSEQPrefs->getPrefInt("SortColumn", preferenceName(),
                m_sortColumn),
            pSEQPrefs->getPrefBool("SortIncreasing", preferenceName(),
                m_sortIncreasing));
}

void SEQListView::setColumnVisible(int column, bool visible)
{
    QString columnName = columnPreferenceName(column);

    // default width is 0
    int width = 0;

    // if column is to become visible, get it's width
    if (visible)
    {
        // get the column width
        width = pSEQPrefs->getPrefInt(columnName + "Width", preferenceName(), 
                columnWidth(column));

        // if it's zero, use default width of 40
        if (width == 0)
            width = 40;
    }

#if QT_VERSION >= 0x050000
    header()->setSectionResizeMode(column, QHeaderView::Interactive);
#else
    header()->setResizeMode(column, QHeaderView::Interactive);
#endif
    header()->resizeSection(column, width);
    setColumnWidth(column, width);

    header()->setSectionHidden(column, !visible);


    // set the the preferences as to if the column is shown
    pSEQPrefs->setPrefBool(QString("Show") + columnName, preferenceName(),
            (width != 0));

    // trigger an update, otherwise things may look messy
    update();
}

void SEQListView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
        emit mouseRightButtonPressed(event);

    QTreeWidget::mousePressEvent(event);
}

#ifndef QMAKEBUILD
#include "seqlistview.moc"
#endif

