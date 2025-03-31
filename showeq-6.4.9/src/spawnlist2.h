/*
 *  spawnlist2.h
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

#ifndef SPAWNLIST2_H
#define SPAWNLIST2_H

#include <QHash>
#include <QMenu>

#include "seqwindow.h"
#include "seqlistview.h"
#include "spawnlistcommon.h"

//--------------------------------------------------
// forward declarations
class Category;
class CategoryMgr;
class Player;
class SpawnShell;
class FilterMgr;

class QComboBox;
class QTimer;
class QSpinBox;

class SpawnListWindow2 : public SEQWindow
{
  Q_OBJECT

 public:
  SpawnListWindow2(Player* player, 
		   SpawnShell* spawnShell, 
		   CategoryMgr* categoryMgr,
		   QWidget* parent = 0, const char* name = 0);
  ~SpawnListWindow2();

   virtual QMenu* menu();


   SpawnListItem* selected();
   SpawnListItem* find(const Item* item);

   QString filterString(const Item* item);

signals:
   void listUpdated();   // flags in spawns have changed
   void listChanged();   // categories have changed
   void spawnSelected(const Item* item);
   void keepUpdated(bool on);

public slots: 
   // SpawnShell signals
   void addItem(const Item *);
   void delItem(const Item *);
   void changeItem(const Item *, uint32_t changeType);
   void killSpawn(const Item *);
   void selectSpawn(const Item *);
   void clear(void);

   // CategoryMgr signals
   void addCategory(const Category* cat);
   void delCategory(const Category* cat);
   void clearedCategories(void);
   void loadedCategories(void);

   // Player signals
   void playerLevelChanged(uint8_t);
   void setPlayer(int16_t x, int16_t y, int16_t z, 
		  int16_t deltaX, int16_t deltaY, int16_t deltaZ, 
		  int32_t degrees); 

   void rebuildSpawnList(void);
   void refresh(void);
   virtual void savePrefs(void);


   void styleChanged();

 private slots:
   // category combo box signals
   void categorySelected(int index);

    // listview signals
   void selChanged();

   void listMouseRightButtonPressed(QMouseEvent* event);
   void listItemPressed(QTreeWidgetItem *litem, int col);
   void listItemDoubleClicked(QTreeWidgetItem *litem, int col);

   // fpm spinbox signals
   void setFPM(int rate);

   // additional menu items
   void toggle_immediateUpdate(bool enable);
   void toggle_keepSorted(bool enable);
   void toggle_keepSelectedVisible(bool enable);
 private:
   void setSelectedQuiet(SEQListViewItem* item, bool selected);
   void populateSpawns(void);
   void populateCategory(const Category* cat);
   void updateCount(void);
   // data sources
   Player *m_player;
   CategoryMgr* m_categoryMgr;
   SpawnShell* m_spawnShell;

   // category currently being viewed
   Category* m_currentCategory;

   // currently selected item
   const Item* m_selectedItem;

   // GUI Items
   QComboBox* m_categoryCombo;
   QSpinBox* m_fpmSpinBox;
   SEQListView* m_spawnList;
   SpawnListMenu* m_menu;
   QLineEdit* m_totalSpawns;

   // index dictionary for retrieving SpawnListItems by Item
   QHash<void*, SpawnListItem*> m_spawnListItemDict;

   // timer used
   QTimer* m_timer;

   // frames per minute (how many times spawnlist is updated per minute)
   int m_delay;
   time_t m_lastUpdate;

   // whether to immediately update the spawn list as changes occur, or
   // use timer.
   bool m_immediateUpdate;
   bool m_keepSorted;
   bool m_keepSelectedVisible;
};

#endif // SPAWNLIST2_H
