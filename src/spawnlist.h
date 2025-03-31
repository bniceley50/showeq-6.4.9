/*
 *  spawnlist.h
 *  Copyright 2000 Maerlyn (MaerlynTheWiz@yahoo.com)
 *  Copyright 2001-2005, 2019 by the respective ShowEQ Developers
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

/*
 * Orig Author - Maerlyn (MaerlynTheWiz@yahoo.com)
 * Date   - 3/16/00
 */

/* 
 * SpawnListItem 
 *
 * SpawnListItem is a class intended to store information about an EverQuest
 * Spawn.  It inherits from QListViewItem but overrides functionality to allow
 * paint styles such as color changes
 *
 * currently it just provides a widget and maintains a QColor for the text
 * display of that widget
 */
 
#ifndef SPAWNLIST_H
#define SPAWNLIST_H

#include <ctime>
#include <cstdio>
#include <sys/time.h>

#include <QHash>
#include <QTextStream>
#include <QMenu>

// these are all used for the CFilterDlg
#include <regex.h>

#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include "seqwindow.h"
#include "seqlistview.h"
#include "spawnlistcommon.h"
#include "spawn.h"

//--------------------------------------------------
// forward declarations
class Category;
class CategoryMgr;
class Item;
class Player;
class SpawnShell;
class FilterMgr;

class SpawnList;
class SpawnListItem;
class SpawnListMenu;

//--------------------------------------------------
// SpawnList
class SpawnList : public SEQListView
{
   Q_OBJECT
public:
   SpawnList(Player* player, 
	     SpawnShell* spawnShell, 
	     CategoryMgr* categoryMgr,
	     QWidget *parent = 0, const char * name = 0);

   SpawnListItem* Selected();
   SpawnListItem* Find(SEQListViewItemIterator& it, 
		       const Item* item, 
		       bool first = false);

   const Category* getCategory(SpawnListItem *);

   SpawnListMenu* menu();

signals:
   void listUpdated();   // flags in spawns have changed
   void listChanged();   // categories have changed
   void spawnSelected(const Item* item);
   void keepUpdated(bool on);

public slots: 
   void setPlayer(int16_t x, int16_t y, int16_t z, 
		  int16_t deltaX, int16_t deltaY, int16_t deltaZ, 
		  int32_t degrees); 
   void selectNext(void);
   void selectPrev(void);
   // SpawnShell signals
   void addItem(const Item *);
   void delItem(const Item *);
   void changeItem(const Item *, uint32_t changeType);
   void killSpawn(const Item *);
   void selectSpawn(const Item *);
   void clear();
   void addCategory(const Category* cat);
   void delCategory(const Category* cat);
   void clearedCategories(void);
   void loadedCategories(void);

   void rebuildSpawnList();
   void playerLevelChanged(uint8_t);

   void styleChanged();

private slots:
   void selChanged();

   void listItemPressed(QTreeWidgetItem* litem, int col);
   void listMouseRightButtonPressed(QMouseEvent* event);
   void listItemDoubleClicked(QTreeWidgetItem* litem, int col);

private:
   void setSelectedQuiet(SEQListViewItem* item, bool selected);
   void populateSpawns(void);
   void populateCategory(const Category* cat);
   QString filterString(const Item *item, int flags = 0);

   void selectAndOpen(SpawnListItem *);
   CategoryMgr* m_categoryMgr;
   Player *m_player;
   SpawnShell* m_spawnShell;

private:
   // category pointer used as keys to look up the associated SpawnListItem
   QHash<void*, SpawnListItem*> m_categoryListItems;

   SpawnListMenu* m_menu;

};

class SpawnListWindow : public SEQWindow
{
  Q_OBJECT

 public:
  SpawnListWindow(Player* player, 
		  SpawnShell* spawnShell, 
		  CategoryMgr* categoryMgr,
		  QWidget* parent = 0, const char* name = 0);
  ~SpawnListWindow();
  virtual QMenu* menu();
  SpawnList* spawnList() { return m_spawnList; }

 protected:
  SpawnList* m_spawnList;

 public slots:
  virtual void savePrefs(void);
  void styleChanged() { m_spawnList->styleChanged(); }

};

#endif // SPAWNLIST_H

