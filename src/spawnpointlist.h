/*
 *  spawnpointlist.h
 *  Borrowed from:  SINS Distributed under GPL
 *  Portions Copyright 2001 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2002-2005, 2019 by the respective ShowEQ Developers
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

#ifndef SPAWNPOINTLIST_H
#define SPAWNPOINTLIST_H

#include <QTimer>
#include <QMenu>

#include "seqlistview.h"
#include "seqwindow.h"
#include "spawnmonitor.h"

// constants
const int tSpawnPointCoord1 = 0;
const int tSpawnPointCoord2 = 1;
const int tSpawnPointCoord3 = 2;
const int tSpawnPointRemaining = 3;
const int tSpawnPointName = 4;
const int tSpawnPointLast = 5;
const int tSpawnPointSpawned = 6;
const int tSpawnPointCount = 7;
const int tSpawnPointMaxCols = 8;

// forward declarations
class SpawnPointList;
class SpawnPointListItem;
class SpawnPointListMenu;
class SpawnPointWindow;

class SpawnPointListItem: public SEQListViewItem
{
public:
  SpawnPointListItem(SEQListView* parent, const SpawnPoint* spawn);
  virtual ~SpawnPointListItem();

  QVariant data(int column, int role) const;

  void update(void);
  const QColor textColor() const { return m_textColor; }
  void setTextColor(const QColor &color);
  const SpawnPoint* spawnPoint() { return m_spawnPoint; }
  bool operator<(const SEQListViewItem& other) const;

 protected:
  QColor m_textColor;
  const SpawnPoint* m_spawnPoint;
};


//--------------------------------------------------
// SpawnListMenu
class SpawnPointListMenu : public QMenu
{
   Q_OBJECT

 public:
  SpawnPointListMenu(SpawnPointList* spawnPointList, 
		     QWidget* parent = 0, const char* name = 0);
  virtual ~SpawnPointListMenu();
  void setCurrentItem(const SpawnPointListItem* item);

 protected slots:
   void init_menu(void);
   void rename_item();
   void delete_item();
   void toggle_col(QAction* col);
   void set_font();
   void set_caption();
   void toggle_keepSorted();

 protected:
  SpawnPointList* m_spawnPointList;
  const SpawnPointListItem* m_currentItem;
  QAction* m_action_rename;
  QAction* m_action_delete;
  QAction* m_action_cols[tSpawnPointMaxCols];
  QAction* m_action_keepSorted;
};

class SpawnPointList : public SEQListView
{
  Q_OBJECT

 public:
  SpawnPointList(SpawnMonitor* spawnMonitor, 
		 QWidget* parent = 0, const char* name = 0);
  SpawnPointListMenu* menu();

  bool keepSorted() { return m_keepSorted; }
  void setKeepSorted(bool val);

 public slots:
  void listMouseRightButtonPressed(QMouseEvent* event);
  void renameItem(const SpawnPointListItem* item);
  void deleteItem(const SpawnPointListItem* item);
  void clearItems(void);
  void refresh();
  void handleSelectItem();
  void newSpawnPoint(const SpawnPoint* sp);
  void clear();
  void handleSelChanged(const SpawnPoint* sp);

 protected:
  SpawnMonitor* m_spawnMonitor;
  SpawnPointListMenu* m_menu;
  QTimer* m_timer;
  bool m_aboutToPop;
  bool m_keepSorted;
};

class SpawnPointWindow : public SEQWindow
{
  Q_OBJECT

 public:
  SpawnPointWindow(SpawnMonitor* spawnMonitor, 
		   QWidget* parent = 0, const char* name = 0);
  ~SpawnPointWindow();
  virtual QMenu* menu();

  SpawnPointList* spawnPointList() { return m_spawnPointList; }

 public slots:
  virtual void savePrefs(void);

 protected:
  SpawnPointList* m_spawnPointList;
};

#endif // SPAWNPOINTLIST_H
