/*
 *  seqlistview.h
 *  Copyright 2001 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2002-2003, 2019 by the respective ShowEQ Developers
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
 *
 */

#ifndef SEQLISTVIEW_H
#define SEQLISTVIEW_H

#include <QTreeWidget>
#include <QMouseEvent>
#include <QString>
#include <QStringList>

typedef QTreeWidgetItem SEQListViewItem;
typedef QTreeWidgetItemIterator SEQListViewItemIterator;

class SEQListView : public QTreeWidget
{
    Q_OBJECT

    public:
        SEQListView(const QString prefName,
                    QWidget* parent = 0,
                    const char* name = 0,
                    Qt::WindowFlags f = Qt::Widget);
        ~SEQListView();

        const QString& preferenceName() const { return m_preferenceName; }
        const QString& columnPreferenceName(int column) const;

        QSizePolicy sizePolicy() const;

        virtual int addColumn(const QString& label,
                              int width = -1);
        virtual int addColumn(const QString& label,
                              const QString& preference,
                              int width = -1);
        virtual void removeColumn(int column);
        void insertItem(QTreeWidgetItem* item);
        bool columnVisible(int column) { return (columnWidth(column) != 0); }
        virtual void setSorting(int column, bool increasing = true);
        int sortColumn() const { return m_sortColumn; }
        bool sortIncreasing() const { return m_sortIncreasing; }

    public slots:
        virtual void restoreColumns(void);
        virtual void savePrefs(void);
        void setColumnVisible(int column, bool visible);
	virtual void setSorting(int column, Qt::SortOrder order);
        void mousePressEvent(QMouseEvent* event);

    signals:
        void mouseRightButtonPressed(QMouseEvent*);

    private:
        QString m_preferenceName;
        QStringList m_columns;
        QStringList m_headers;
        int m_sortColumn;
        bool m_sortIncreasing;
};

#endif // SEQLISTVIEW_H
