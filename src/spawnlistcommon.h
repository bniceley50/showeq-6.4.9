/*
 *  spawnlistcommon.h
 *  Copyright 2000 Maerlyn (MaerlynTheWiz@yahoo.com)
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

/*
 * Orig Author - Maerlyn (MaerlynTheWiz@yahoo.com)
 * Date   - 3/16/00
 */

#ifndef SPAWNLISTCOMMON_H
#define SPAWNLISTCOMMON_H

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif

#include <QString>
#include <QMenu>

#include "spawn.h"
#include "seqlistview.h"


//--------------------------------------------------
// forward declarations
class Category;
class CategoryMgr;
class SpawnShell;
class FilterMgr;
class SEQListView;
class SEQWindow;
class Player;

class SpawnListItem;
class SpawnListMenu;

//--------------------------------------------------
// constants
const int tSpawnColName = 0;
const int tSpawnColLevel = 1;
const int tSpawnColHP = 2;
const int tSpawnColMaxHP = 3;
const int tSpawnColXPos = 4;
const int tSpawnColYPos = 5;
const int tSpawnColZPos = 6;
const int tSpawnColID   = 7;
const int tSpawnColDist = 8;
const int tSpawnColRace = 9;
const int tSpawnColClass = 10;
const int tSpawnColInfo = 11;
const int tSpawnColSpawnTime =12;
const int tSpawnColDeity = 13;
const int tSpawnColBodyType = 14;
const int tSpawnColGuildID = 15;
const int tSpawnColMaxCols = 16;

//--------------------------------------------------
// SpawnListItem
class SpawnListItem : public SEQListViewItem
{
public:
   SpawnListItem(SEQListViewItem *parent);
   SpawnListItem(SEQListView *parent);
   virtual ~SpawnListItem();

   const QColor textColor()  { return m_textColor; }
   void setTextColor(const QColor &color) { m_textColor = color; }
   void pickTextColor(const Item* item,
		      Player* player, 
		      QColor def = Qt::black);
   const Item*    item() const { return m_item; }

   void update(Player* player, uint32_t changeType);
   void updateTitle(const QString& name);
   void setShellItem(const Item *);
   spawnItemType type();

   QVariant data(int column, int role) const;
   bool operator<(const SEQListViewItem& other) const;

   //--------------------------------------------------
   int m_npc;
private:
   QColor m_textColor;

   const Item *m_item;
};

//--------------------------------------------------
// SpawnListMenu
class SpawnListMenu : public QMenu
{
   Q_OBJECT

 public:
  SpawnListMenu(SEQListView* spawnlist,
		SEQWindow* spawnlistWindow,
		FilterMgr* filterMgr,
		CategoryMgr* categoryMgr,
		QWidget* parent = 0, const char* name = 0);
  virtual ~SpawnListMenu();
  void setCurrentCategory(const Category* cat);
  void setCurrentItem(const Item* item);

 protected slots:
   void init_Menu(void);
   void toggle_spawnListCol( QAction* col );
   void add_filter(QAction* selection);
   void add_zoneFilter(QAction* selection);
   void add_category();
   void edit_category();
   void delete_category();
   void reload_categories();
   void set_font();
   void set_caption();

 protected:
  SEQListView* m_spawnlist;
  SEQWindow* m_spawnlistWindow;
  FilterMgr* m_filterMgr;
  CategoryMgr* m_categoryMgr;
  const Category* m_currentCategory;
  const Item* m_currentItem;
  QMenu* m_filterMenu;
  QMenu* m_zoneFilterMenu;
  QAction* m_action_spawnList_Cols[tSpawnColMaxCols];
  QAction* m_action_edit_category;
  QAction* m_action_delete_category;
};

#endif // SPAWNLISTCOMMON_H
