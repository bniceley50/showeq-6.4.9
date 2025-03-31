/*
 *  spawnlistcommon.cpp
 *  Copyright 2000 Maerlyn (MaerlynTheWiz@yahoo.com)
 *  Copyright 2000-200, 2019 by the respective ShowEQ Developers
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

#include "seqwindow.h"
#include "seqlistview.h"
#include "spawnlistcommon.h"
#include "category.h"
#include "filtermgr.h"
#include "spawnshell.h"
#include "main.h"
#include "player.h"
#include "diagnosticmessages.h"
#include "filterlistwindow.h"

#include <cstring>

#include <QApplication>
#include <QFontDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFont>
#include <QPainter>
#include <QMenu>

SpawnListItem::SpawnListItem(SEQListViewItem *parent) : SEQListViewItem(parent)
{
  m_textColor = qApp->palette().color(QPalette::WindowText);

  m_item = NULL;
  m_npc = 0;
}

SpawnListItem::SpawnListItem(SEQListView *parent) : SEQListViewItem(parent)
{
  m_textColor = qApp->palette().color(QPalette::WindowText);

  m_item = NULL;
  m_npc = 0;
}

SpawnListItem::~SpawnListItem()
{
}

QVariant SpawnListItem::data(int column, int role) const
{
    QFont font = treeWidget()->font();
    QFontMetrics metrics(font);
    uint32_t filterFlags = 0;
    if (m_item)
        filterFlags = m_item->filterFlags();

    switch(role)
    {
        case Qt::FontRole:
            if (!(filterFlags & (
                            FILTER_FLAG_FILTERED |
                            FILTER_FLAG_ALERT |
                            FILTER_FLAG_LOCATE |
                            FILTER_FLAG_CAUTION |
                            FILTER_FLAG_DANGER)))
            {
                font.setBold(false);
                font.setItalic(false);
                font.setUnderline(false);
            }
            else
            {
                if (filterFlags & FILTER_FLAG_ALERT)
                    font.setBold(true);
                else
                    font.setBold(false);

                if (filterFlags & FILTER_FLAG_LOCATE)
                    font.setItalic(true);
                else
                    font.setItalic(false);

                if ((filterFlags & FILTER_FLAG_CAUTION) ||
                        (filterFlags & FILTER_FLAG_DANGER))
                    font.setUnderline(true);
                else
                    font.setUnderline(false);
            }

            return font;

        case Qt::ForegroundRole:
            if (filterFlags & FILTER_FLAG_FILTERED)
                // color filtered spawns grey
                return QColor(Qt::gray);
            else
                return m_textColor;

        case Qt::SizeHintRole:
            return QSize(-1, metrics.lineSpacing());

        default:
            return SEQListViewItem::data(column, role);
    }

}

spawnItemType SpawnListItem::type()
{
   return item() ? item()->type() : tUnknown;
}

bool SpawnListItem::operator<(const SEQListViewItem& other) const
{
    int column = treeWidget() ? treeWidget()->sortColumn() : 0;

    switch(column)
    {
        case 1: // level
        case 2: // hp
        case 3: // max hp
        case 4: // coord 1
        case 5: // coord 2
        case 6: // coord 3
        case 7: // ID
            return data(column, Qt::DisplayRole).value<int>() <
                other.data(column, Qt::DisplayRole).value<int>();

        case 8: // distance
            // "fast machine" option will use float, not int
            return
                ((data(column, Qt::DisplayRole).canConvert<float>()) ?
                data(column, Qt::DisplayRole).value<float>() :
                data(column, Qt::DisplayRole).value<int>())
                <
                ((other.data(column, Qt::DisplayRole).canConvert<float>()) ?
                other.data(column, Qt::DisplayRole).value<float>() :
                other.data(column, Qt::DisplayRole).value<int>());

        case 0: // name
        case 9: // race
        case 10:// class
        case 11:// info
        case 12:// spawn time
        case 13:// diety
        case 14:// body type
        case 15:// guild tag
        default: // Qt sorts values as strings by default
            return text(column) < other.text(column);
    }

}

void SpawnListItem::update(Player* player, uint32_t changeType)
{
//   seqDebug("SpawnListItem::update()\n");
   QString buff;
   const Spawn* spawn = NULL;

   if ((item()->type() == tSpawn) || (item()->type() == tPlayer))
     spawn = (const Spawn*)item();

   if (changeType & tSpawnChangedName)
   {
     // Name
     if (!showeq_params->showRealName)
       buff = item()->transformedName();
     else
       buff = item()->name();

     if (spawn != NULL) 
     {
       if (!spawn->lastName().isEmpty())
       {
         buff = QString("%1 (%2)").arg(buff).arg(spawn->lastName());
       }
       if (spawn->gm())
         buff += " *GM* ";
     }

     setText(tSpawnColName, buff);
   }

   // only spawns contain level info
   if (spawn != NULL)
   {
     if (changeType & tSpawnChangedLevel)
     {
       // Level
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
       buff = QString::asprintf("%2d", spawn->level());
#else
       buff.sprintf("%2d", spawn->level());
#endif
       setText(tSpawnColLevel, buff);
     }
     
     if (changeType & tSpawnChangedHP)
     {
       // Hitpoints
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
       buff = QString::asprintf("%5d", spawn->HP());
#else
       buff.sprintf("%5d", spawn->HP());
#endif
       setText(tSpawnColHP, buff);
       
       // Maximum Hitpoints
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
       buff = QString::asprintf("%5d", spawn->maxHP());
#else
       buff.sprintf("%5d", spawn->maxHP());
#endif
       setText(tSpawnColMaxHP, buff);
     }

     if (changeType == tSpawnChangedALL)
     {
       setText(tSpawnColDeity, spawn->deityName());
       setText(tSpawnColBodyType, spawn->typeString());
       if (!spawn->guildTag().isEmpty())
            setText(tSpawnColGuildID, spawn->guildTag());
       else if (spawn->guildID())
            setText(tSpawnColGuildID, QString::number(spawn->guildID()));
       else
           setText(tSpawnColGuildID, " ");
     }
   }
   else if (changeType == tSpawnChangedALL)
   {
     buff = "0";
     setText(tSpawnColLevel, buff);
     setText(tSpawnColHP, buff);
     setText(tSpawnColMaxHP, buff);
   }

   if (changeType & tSpawnChangedPosition)
   {
     // X position
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     buff = QString::asprintf("%5d", showeq_params->retarded_coords ? 
		  (int)item()->y() : (int)item()->x());
#else
     buff.sprintf("%5d", showeq_params->retarded_coords ? 
		  (int)item()->y() : (int)item()->x());
#endif
     setText(tSpawnColXPos, buff);
     
     // Y position
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     buff = QString::asprintf("%5d", showeq_params->retarded_coords ? 
		  (int)item()->x() : (int)item()->y());
#else
     buff.sprintf("%5d", showeq_params->retarded_coords ? 
		  (int)item()->x() : (int)item()->y());
#endif
     setText(tSpawnColYPos, buff);
     
     // Z position
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     buff = QString::asprintf("%5d", item()->z());
#else
     buff.sprintf("%5d", item()->z());
#endif
     setText(tSpawnColZPos, buff);

     // Distance
     if (!showeq_params->fast_machine)
     {
       //buff.sprintf("%5d", player->calcDist2DInt(*item()));
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
       buff = QString::asprintf("%5d", item()->getIDistanceToPlayer());
#else
       buff.sprintf("%5d", item()->getIDistanceToPlayer());
#endif
     }
     else
     {
       //buff.sprintf("%5.1f", player->calcDist(*item()));
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
       buff = QString::asprintf("%5.1f", item()->getFDistanceToPlayer());
#else
       buff.sprintf("%5.1f", item()->getFDistanceToPlayer());
#endif
     }
     setText(tSpawnColDist, buff);
   }

   if (changeType == tSpawnChangedALL)
   {
     // Id
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     buff = QString::asprintf("%5d", item()->id());
#else
     buff.sprintf("%5d", item()->id());
#endif
     setText(tSpawnColID, buff);
     
     // Race
     setText(tSpawnColRace, item()->raceString());
     
     // Class
     setText(tSpawnColClass, item()->classString());
     
     // Spawntime
     setText(tSpawnColSpawnTime, m_item->spawnTimeStr());

     // CJD TODO - Deity, PVP teams
   }

   if (changeType & tSpawnChangedWearing)
   {
     // Info
     setText(tSpawnColInfo, item()->info());
   }

   m_npc = item()->NPC();
}

void SpawnListItem::updateTitle(const QString& name)
{
  // update childcount in header
  QString temp;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  temp = QString::asprintf("%s (%d)",
          name.toLatin1().data(), childCount());
#else
  temp.sprintf("%s (%d)",
          name.toLatin1().data(), childCount());
#endif

  setText(tSpawnColName, temp);
} // end if spawn should be in this category

void SpawnListItem::setShellItem(const Item *item)
{
   m_item = item;
   if (item)
      m_npc = item->NPC();
}

//----------------------------------------------------------------------
//
// pickTextColor
// 
// insert color schemes here
//
void SpawnListItem::pickTextColor(const Item* item, 
				  Player* player, 
				  QColor def)
{

  QColor fg = qApp->palette().color(QPalette::WindowText);
  QColor bg = qApp->palette().color(QPalette::Base);

  //Black is the parameter default, so if it's black, we should use the
  //foreground color instead.  That way we won't wind up with black text by
  //default when using dark themesf
  if (def == Qt::black)
      def = fg;

  if (item == NULL)
  {
    m_textColor = def;
    return;
  }

  const Spawn* spawn = NULL;
  if ((item->type() == tSpawn) || (item->type() == tPlayer))
    spawn = (const Spawn*)item;

  if (spawn == NULL)
  {
    m_textColor = def;
    return;
  }

  switch (spawn->typeflag())
  {
  case 65:
    m_textColor = Qt::magenta;
    return;
  case 66:
  case 67:
  case 100:
    m_textColor = Qt::darkMagenta;
    return;
  }

  // color by pvp team
  if (showeq_params->pvp) // if pvp
  {
    switch(spawn->raceTeam()) 
    {
    case RTEAM_HUMAN:
      m_textColor = Qt::blue;
      return;
    case RTEAM_ELF:
      m_textColor = QColor(196,206,12);
      return;
    case RTEAM_DARK:
      m_textColor = QColor(206,151,33);
      return;
    case RTEAM_SHORT:
      m_textColor = Qt::magenta;
      return;
    }
  } 
  else if (showeq_params->deitypvp) // if deitypvp
  {
    switch(spawn->deityTeam()) 
    {
    case DTEAM_GOOD:
      m_textColor = Qt::blue;
      return;
    case DTEAM_NEUTRAL:
      m_textColor = QColor(196,206,12);
      return;
    case DTEAM_EVIL:
      m_textColor = Qt::magenta;
      return;
    }
  }

  // color by consider difficulty
  m_textColor = player->pickConColor(spawn->level());
  if (m_textColor == Qt::white || m_textColor == Qt::black)
    m_textColor = def;
  if (m_textColor == Qt::yellow)
    m_textColor = QColor(206,151,33);
} // end pickTextColor

SpawnListMenu::SpawnListMenu(SEQListView* spawnlist,
			     SEQWindow* spawnlistWindow,
			     FilterMgr* filterMgr,
			     CategoryMgr* categoryMgr,
		             QWidget* parent, const char* name)
  : m_spawnlist(spawnlist),
    m_spawnlistWindow(spawnlistWindow),
    m_filterMgr(filterMgr),
    m_categoryMgr(categoryMgr)
{
  // Show Columns
  QMenu* spawnListColMenu = new QMenu("Show &Column");
  addMenu(spawnListColMenu);

  m_action_spawnList_Cols[tSpawnColName] = spawnListColMenu->addAction("&Name");
  m_action_spawnList_Cols[tSpawnColName]->setData(tSpawnColName);
  m_action_spawnList_Cols[tSpawnColName]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColLevel] = spawnListColMenu->addAction("&Level");
  m_action_spawnList_Cols[tSpawnColLevel]->setData(tSpawnColLevel);
  m_action_spawnList_Cols[tSpawnColLevel]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColHP] = spawnListColMenu->addAction("&HP");
  m_action_spawnList_Cols[tSpawnColHP]->setData(tSpawnColHP);
  m_action_spawnList_Cols[tSpawnColHP]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColMaxHP] = spawnListColMenu->addAction("&Max HP");
  m_action_spawnList_Cols[tSpawnColMaxHP]->setData(tSpawnColMaxHP);
  m_action_spawnList_Cols[tSpawnColMaxHP]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColXPos] = spawnListColMenu->addAction("Coord &1");
  m_action_spawnList_Cols[tSpawnColXPos]->setData(tSpawnColXPos);
  m_action_spawnList_Cols[tSpawnColXPos]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColYPos] = spawnListColMenu->addAction("Coord &2");
  m_action_spawnList_Cols[tSpawnColYPos]->setData(tSpawnColYPos);
  m_action_spawnList_Cols[tSpawnColYPos]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColZPos] = spawnListColMenu->addAction("Coord &3");
  m_action_spawnList_Cols[tSpawnColZPos]->setData(tSpawnColZPos);
  m_action_spawnList_Cols[tSpawnColZPos]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColID] = spawnListColMenu->addAction("I&D");
  m_action_spawnList_Cols[tSpawnColID]->setData(tSpawnColID);
  m_action_spawnList_Cols[tSpawnColID]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColDist] = spawnListColMenu->addAction("&Dist");
  m_action_spawnList_Cols[tSpawnColDist]->setData(tSpawnColDist);
  m_action_spawnList_Cols[tSpawnColDist]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColRace] = spawnListColMenu->addAction("&Race");
  m_action_spawnList_Cols[tSpawnColRace]->setData(tSpawnColRace);
  m_action_spawnList_Cols[tSpawnColRace]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColClass] = spawnListColMenu->addAction("&Class");
  m_action_spawnList_Cols[tSpawnColClass]->setData(tSpawnColClass);
  m_action_spawnList_Cols[tSpawnColClass]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColInfo] = spawnListColMenu->addAction("&Info");
  m_action_spawnList_Cols[tSpawnColInfo]->setData(tSpawnColInfo);
  m_action_spawnList_Cols[tSpawnColInfo]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColSpawnTime] = spawnListColMenu->addAction("Spawn &Time");
  m_action_spawnList_Cols[tSpawnColSpawnTime]->setData(tSpawnColSpawnTime);
  m_action_spawnList_Cols[tSpawnColSpawnTime]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColDeity] = spawnListColMenu->addAction("&Deity");
  m_action_spawnList_Cols[tSpawnColDeity]->setData(tSpawnColDeity);
  m_action_spawnList_Cols[tSpawnColDeity]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColBodyType] = spawnListColMenu->addAction("&Body Type");
  m_action_spawnList_Cols[tSpawnColBodyType]->setData(tSpawnColBodyType);
  m_action_spawnList_Cols[tSpawnColBodyType]->setCheckable(true);

  m_action_spawnList_Cols[tSpawnColGuildID] = spawnListColMenu->addAction("GuildTag");
  m_action_spawnList_Cols[tSpawnColGuildID]->setData(tSpawnColGuildID);
  m_action_spawnList_Cols[tSpawnColGuildID]->setCheckable(true);


  connect (spawnListColMenu, SIGNAL(triggered(QAction*)),
	   this, SLOT(toggle_spawnListCol(QAction*)));

  QAction* tmpAction;
  m_filterMenu = new QMenu("Add &Filter");
  addMenu(m_filterMenu);
  m_filterMenu->setEnabled(false);
  tmpAction = m_filterMenu->addAction("&Hunt...");
  tmpAction->setData(HUNT_FILTER);
  tmpAction = m_filterMenu->addAction("&Caution...");
  tmpAction->setData(CAUTION_FILTER);
  tmpAction = m_filterMenu->addAction("&Danger...");
  tmpAction->setData(DANGER_FILTER);
  tmpAction = m_filterMenu->addAction("&Locate...");
  tmpAction->setData(LOCATE_FILTER);
  tmpAction = m_filterMenu->addAction("&Alert...");
  tmpAction->setData(ALERT_FILTER);
  tmpAction = m_filterMenu->addAction("&Filtered...");
  tmpAction->setData(FILTERED_FILTER);
  tmpAction = m_filterMenu->addAction("&Tracer...");
  tmpAction->setData(TRACER_FILTER);
  connect (m_filterMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(add_filter(QAction*)));


  m_zoneFilterMenu = new QMenu("Add &Zone Filter");
  addMenu(m_zoneFilterMenu);
  m_zoneFilterMenu->setEnabled(false);
  tmpAction = m_zoneFilterMenu->addAction("&Hunt...");
  tmpAction->setData(HUNT_FILTER);
  tmpAction = m_zoneFilterMenu->addAction("&Caution...");
  tmpAction->setData(CAUTION_FILTER);
  tmpAction = m_zoneFilterMenu->addAction("&Danger...");
  tmpAction->setData(DANGER_FILTER);
  tmpAction = m_zoneFilterMenu->addAction("&Locate...");
  tmpAction->setData(LOCATE_FILTER);
  tmpAction = m_zoneFilterMenu->addAction("&Alert...");
  tmpAction->setData(ALERT_FILTER);
  tmpAction = m_zoneFilterMenu->addAction("&Filtered...");
  tmpAction->setData(FILTERED_FILTER);
  tmpAction = m_zoneFilterMenu->addAction("&Tracer...");
  tmpAction->setData(TRACER_FILTER);
  connect (m_zoneFilterMenu, SIGNAL(triggered(QAction*)),
	   this, SLOT(add_zoneFilter(QAction*)));

  addSeparator();

  addAction("&Add Category...", this, SLOT(add_category()));
  m_action_edit_category = addAction("&Edit Category...", this, SLOT(edit_category()));
  m_action_delete_category = addAction("&Delete Category...", this, SLOT(delete_category()));
  addAction("&Reload Categories", this, SLOT(reload_categories()));
  addSeparator();
  addAction("&Font...", this, SLOT(set_font()));
  addAction("&Caption...", this, SLOT(set_caption()));

  connect(this, SIGNAL(aboutToShow()),
	  this, SLOT(init_Menu()));
}

SpawnListMenu::~SpawnListMenu()
{
}

void SpawnListMenu::init_Menu(void)
{
  // make sure the menu bar settings are correct
  for (int i = 0; i < tSpawnColMaxCols; i++)
      m_action_spawnList_Cols[i]->setChecked(m_spawnlist->columnVisible(i));

}

void SpawnListMenu::setCurrentCategory(const Category* cat)
{
  // set the current category
  m_currentCategory = cat;

  // update the menu item names
  if (cat != NULL)
  {
    m_action_edit_category->setText(
            "&Edit '" + cat->name() + "' Category...");
    m_action_edit_category->setEnabled(true);

    m_action_delete_category->setText(
            "&Delete '" + cat->name() + "' Category...");
    m_action_delete_category->setEnabled(true);
  }
  else
  {
    m_action_edit_category->setText("&Edit Category...");
    m_action_edit_category->setEnabled(false);

    m_action_delete_category->setText("&Delete Category...");
    m_action_delete_category->setEnabled(false);
  }
}

void SpawnListMenu::setCurrentItem(const Item* item)
{
  // set the current item
  m_currentItem = item;

  // enable/disable item depending on if there is one
  m_filterMenu->setEnabled(item != NULL);
  m_zoneFilterMenu->setEnabled(item != NULL);

  if (item != NULL)
  {
    m_filterMenu->setTitle("Add '" + item->name() + "' &Filter");
    m_zoneFilterMenu->setTitle("Add '" + item->name() + "' &Zone Filter");
  }
  else
  {
    m_filterMenu->setTitle("Add &Filter");
    m_zoneFilterMenu->setTitle("Add &Zone Filter");
  }
}

void SpawnListMenu::toggle_spawnListCol(QAction* col)
{
  int colnum = col->data().value<int>();
  m_spawnlist->setColumnVisible(colnum, col->isChecked());
}

void SpawnListMenu::add_filter(QAction* selection)
{
  if (m_currentItem == NULL)
    return;

  int filter = selection->data().value<int>();
  QString filterName = m_filterMgr->filterName(filter);
  QString filterString = m_currentItem->filterString();

  // get the user edited filter string, based on the items filterString
  bool ok = false;
  filterString =
    FilterDialog::getFilter(m_spawnlist, "Add " + filterName + " Filter",
            filterString, &ok);

  // if the user clicked ok, add the filter
  if (ok)
    m_filterMgr->addFilter(filter, filterString);
}

void SpawnListMenu::add_zoneFilter(QAction* selection)
{
  if (m_currentItem == NULL)
    return;

  int filter = selection->data().value<int>();
  QString filterName = m_filterMgr->filterName(filter);
  QString filterString = m_currentItem->filterString();

  // get the user edited filter string, based on the items filterString
  bool ok = false;
  filterString =
    FilterDialog::getFilter(m_spawnlist, "Add " + filterName + " Zone Filter",
            filterString, &ok);

  // if the user clicked ok, add the filter
  if (ok)
    m_filterMgr->addZoneFilter(filter, filterString);
}

void SpawnListMenu::add_category()
{
  // add a category to the category manager
  m_categoryMgr->addCategory(m_spawnlist);
}

void SpawnListMenu::edit_category()
{
  // edit the current category
  m_categoryMgr->editCategories(m_currentCategory, m_spawnlist);
}

void SpawnListMenu::delete_category()
{
  // confirm that the user wants to delete the category
  QMessageBox mb("Are you sure?",
		 "Are you sure you wish to delete category "
		 + m_currentCategory->name() + "?",
		 QMessageBox::NoIcon,
		 QMessageBox::Yes, 
		 QMessageBox::No | QMessageBox::Default | QMessageBox::Escape,
		 QMessageBox::NoButton,
		 m_spawnlist);
  
  // if user chose yes, then delete the category
  if (mb.exec() == QMessageBox::Yes)
    m_categoryMgr->remCategory(m_currentCategory);
}

void SpawnListMenu::reload_categories()
{
  // reload the categories
  m_categoryMgr->reloadCategories();
}


void SpawnListMenu::set_font()
{
  QFont newFont;
  bool ok = false;
  // get a new font
  newFont = QFontDialog::getFont(&ok, m_spawnlistWindow->font(),
				 this, QString("ShowEQ Spawn List Font"));
    
    
    // if the user entered a font and clicked ok, set the windows font
    if (ok)
      m_spawnlistWindow->setWindowFont(newFont);
}

void SpawnListMenu::set_caption()
{
  bool ok = false;

  QString caption =
    QInputDialog::getText(this, "ShowEQ Spawn List Window Caption",
            "Enter caption for the Spawn List Window:",
            QLineEdit::Normal, m_spawnlistWindow->windowTitle(),
            &ok);

  // if the user entered a caption and clicked ok, set the windows caption
  if (ok)
    m_spawnlistWindow->setWindowTitle(caption);
}

#ifndef QMAKEBUILD
#include "spawnlistcommon.moc"
#endif

