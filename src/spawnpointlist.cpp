/*
 *  spawnpointlist.cpp
 *  Borrowed from:  SINS Distributed under GPL
 *  Portions Copyright 2001,2007 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2002-2007, 2019 by the respective ShowEQ Developers
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

#include <ctime>

#include <QInputDialog>
#include <QMessageBox>
#include <QFontDialog>
#include <QPainter>
#include <QLayout>
#include <QMenu>
#include <QHeaderView>

#include "spawnpointlist.h"
#include "main.h"

SpawnPointListItem::SpawnPointListItem(SEQListView* parent, const SpawnPoint* sp)
  : SEQListViewItem( parent )
{
#ifdef DEBUG
//      debug( "SpawnItem::SpawnItem()" );
#endif
  m_textColor = Qt::black;
  m_spawnPoint = sp;

  update();
}

SpawnPointListItem::~SpawnPointListItem()
{
}

void SpawnPointListItem::update()
{
  QString tmpStr;
  // set the coordinate information
  if (showeq_params->retarded_coords)
  {
    setText(tSpawnPointCoord1, QString::number(m_spawnPoint->y()));
    setText(tSpawnPointCoord2, QString::number(m_spawnPoint->x()));
  }
  else
  {
    setText(tSpawnPointCoord1, QString::number(m_spawnPoint->x()));
    setText(tSpawnPointCoord2, QString::number(m_spawnPoint->y()));
  }
  setText(tSpawnPointCoord3, 
	  QString::number(m_spawnPoint->z(), 'f', 1));

  // construct and set the time remaining string
  if ( m_spawnPoint->diffTime() == 0 || m_spawnPoint->deathTime() == 0 )
    tmpStr = "\277 ?";  // upside down questoin mark followed by question mark
  else
  {
    long secs = m_spawnPoint->secsLeft();
    
    if ( secs > 0 )
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
      tmpStr = QString::asprintf( "%2ld:%02ld", secs / 60, secs % 60  );
#else
      tmpStr.sprintf( "%2ld:%02ld", secs / 60, secs % 60  );
#endif
    else
      tmpStr = "   now"; // spaces followed by now
  }
  setText(tSpawnPointRemaining, tmpStr);

  // set the name and last spawn info
  setText(tSpawnPointName, m_spawnPoint->name());
  setText(tSpawnPointLast, m_spawnPoint->last());

  // construct and set the spawned string
  QDateTime       dateTime;
  dateTime.setTime_t( m_spawnPoint->spawnTime() );
  QDate           createDate = dateTime.date();
  tmpStr = "";
  // spawn time
  if ( createDate != QDate::currentDate() )
    tmpStr = createDate.shortDayName( createDate.dayOfWeek() ) + " ";
  
  tmpStr += dateTime.time().toString();

  // set when it spawned and the count
  setText(tSpawnPointSpawned, tmpStr);
  setText(tSpawnPointCount, QString::number(m_spawnPoint->count()));
}

void SpawnPointListItem::setTextColor( const QColor& color )
{
  m_textColor = color;
  update();
}


QVariant SpawnPointListItem::data(int column, int role) const
{
    QFont font = treeWidget()->font();
    switch(role)
    {
        case Qt::FontRole:
            if (m_spawnPoint->age() > 220)
                font.setBold(true);
            else
                font.setBold(false);

            return font;

        case Qt::ForegroundRole:
            if (m_spawnPoint->age() > 220)
                return QColor(Qt::red);
            else
                return treeWidget()->foregroundRole();

        default:
            return SEQListViewItem::data(column, role);
    }

}

bool SpawnPointListItem::operator<(const SEQListViewItem& other) const
{
    int column = treeWidget() ? treeWidget()->sortColumn() : 0;

    switch(column)
    {
        case 1: // coord 1
        case 2: // coord 2
        case 3: // coord 3
        case 8: // count
            return data(column, Qt::DisplayRole).value<int>() <
                other.data(column, Qt::DisplayRole).value<int>();

        case 4: // remaining
        case 5: // name
        case 6: // last
        case 7: // spawned
        default: // Qt sorts values as strings by default
            return text(column) < other.text(column);
    }

}


SpawnPointList::SpawnPointList(SpawnMonitor* spawnMonitor, 
			       QWidget* parent, const char* name):
  SEQListView("SpawnPointList", parent, name),
  m_spawnMonitor(spawnMonitor),
  m_menu(NULL),
  m_aboutToPop(false)
{
  // get whether to keep the list sorted or not
  m_keepSorted = pSEQPrefs->getPrefBool("KeepSorted", preferenceName(), false);
  setSortingEnabled(m_keepSorted);

  // add all the columns
  if (showeq_params->retarded_coords)
  {
    addColumn ("N/S", "Coord1");
    addColumn ("E/W", "Coord2");
  }
  else
  {
    addColumn ("X", "Coord1");
    addColumn ("Y", "Coord2");
  }
  addColumn("Z", "Coord3");
  addColumn("Remaining");
  addColumn("Name");
  addColumn("Last");
  addColumn("Spawned");
  addColumn("Count");

  // default sort is on remaining
  setSorting(tSpawnPointRemaining, true);

  // restore the columns settings from preferences
  restoreColumns();

  // put in all the spawn points that might already be present in
  // the spawn monitor
  QHashIterator<QString, SpawnPoint*> it( m_spawnMonitor->spawnPoints() );
  SpawnPoint* sp;
  while (it.hasNext())
  {
    it.next();
    sp = it.value();
    new SpawnPointListItem(this, sp);
  }

  // create the timer
  m_timer = new QTimer(this);
  m_timer->setObjectName("spawn-point-timer");
  connect( m_timer, SIGNAL( timeout() ), this, SLOT( refresh() ) );


  connect(m_spawnMonitor, SIGNAL(newSpawnPoint(const SpawnPoint*)),
	  this, SLOT(newSpawnPoint(const SpawnPoint*)));
  connect(m_spawnMonitor, SIGNAL(clearSpawnPoints()),
	  this, SLOT(clear()));
  connect(m_spawnMonitor, SIGNAL(selectionChanged(const SpawnPoint*)),
	  this, SLOT(handleSelChanged(const SpawnPoint*)));
  connect(this, SIGNAL(mouseRightButtonPressed(QMouseEvent*)),
	  this, SLOT(listMouseRightButtonPressed(QMouseEvent*)));
  connect(this, SIGNAL( itemSelectionChanged()),
	  this, SLOT(handleSelectItem()));
  m_timer->start(10000L);
}

void SpawnPointList::setKeepSorted(bool val)
{
  m_keepSorted = val;
  pSEQPrefs->setPrefBool("KeepSorted", preferenceName(), 
			 m_keepSorted);
  setSortingEnabled(m_keepSorted);
}

void SpawnPointList::handleSelectItem()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  if (!selected.count()) return;

  // the list is limited to one selection at a time, so we can take the first
  SEQListViewItem* item = selected.first();
  m_menu->setCurrentItem((SpawnPointListItem*)item);
  if (item == NULL) return;


  const SpawnPoint* sp = NULL;
  
  if ( item )
  {
    // get the spawn point associated with the list view item
    sp = ((SpawnPointListItem*)item)->spawnPoint();

    // set the selected spawn point
    m_spawnMonitor->setSelected(sp);
  }
}

void SpawnPointList::handleSelChanged(const SpawnPoint* sp)
{
  SEQListViewItemIterator it(this);

  SpawnPointListItem* splitem;
  
  // keep iterating until we find a match
  while (*it)
  {
    splitem = (SpawnPointListItem*)*it;

    // is this the current item
    if (splitem->spawnPoint() == sp)
    {
      // yes, set it as the selected item in the spawn list
      setCurrentItem(splitem);

      // ensure that the item is visible
      scrollToItem(splitem);

      break;
    }
    ++it;
  }
}

void SpawnPointList::listMouseRightButtonPressed(QMouseEvent* event) {
    if (event->button() == Qt::RightButton)
    {
        // popup a context-menu
        SpawnPointListMenu* spawnPointMenu = (SpawnPointListMenu*)menu();
        spawnPointMenu->popup(event->globalPos());
    }
}

void SpawnPointList::renameItem(const SpawnPointListItem* item)
{
  if (item == NULL)
    return;

  // get the underlying spawn point
  const SpawnPoint* sp = ((SpawnPointListItem*)item)->spawnPoint();


  if ( sp )
  {
    // default to the existing name
    QString def = sp->name();

    // if there is no existing name, use the name of the last spawn
    if (def.isEmpty())
      def = sp->last();

    // ask the user for the new name
    bool ok = false;
    QString text = QInputDialog::getText(
            this,
            tr("Spawn Point"),
            tr("New name:"),
            QLineEdit::Normal,
            def,
            &ok);

    // if the user clicked ok and they entered a name, set the new name
    if (ok && !text.isEmpty())
    {
      m_spawnMonitor->setName(sp, text);
      ((SpawnPointListItem*)item)->update();
    }
  }
}

void SpawnPointList::deleteItem(const SpawnPointListItem* item)
{
  if (item == NULL)
    return;

  // get the underlying spawn point
  const SpawnPoint* sp = ((SpawnPointListItem*)item)->spawnPoint();


  if (sp == NULL)
    return;

  // default to the existing name
  QString def = sp->name();

  // if there is no existing name, use the name of the last spawn
  if (def.isEmpty())
    def = sp->last();

#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  def = QString::asprintf("%d/%d/%d '%s'",
          sp->x(), sp->y(), sp->z(), def.toLatin1().data());
#else
  def.sprintf("%d/%d/%d '%s'",
          sp->x(), sp->y(), sp->z(), def.toLatin1().data());
#endif

  // confirm that the user wants to delete the category
  QMessageBox mb("Are you sure?",
		 "Are you sure you wish to delete spawn point "
		 + def + "?",
		 QMessageBox::NoIcon,
		 QMessageBox::Yes, 
		 QMessageBox::No | QMessageBox::Default | QMessageBox::Escape,
		 QMessageBox::NoButton,
		 this);
  
  // if user chose eys, then delete the spawn point
  if (mb.exec() == QMessageBox::Yes)
  {
    // remove the item from the spawn point list
    delete item;
    
    // remove the item from the spawn monitor
    m_spawnMonitor->deleteSpawnPoint(sp);
  }
}

void SpawnPointList::clearItems(void)
{
  // confirm that the user wants to delete the category
  QMessageBox mb("Are you sure?",
		 "Are you sure you wish to clear all the spawn points?",
		 QMessageBox::NoIcon,
		 QMessageBox::Yes, 
		 QMessageBox::No | QMessageBox::Default | QMessageBox::Escape,
		 QMessageBox::NoButton,
		 this);
  
  // if user chose eys, then clear the spawn points
  if (mb.exec() == QMessageBox::Yes)
    m_spawnMonitor->clear();
}

void SpawnPointList::refresh()
{
  bool aboutToPop = false;

  // iterate over all the spawn point list items and update them
  SEQListViewItemIterator lit(this);
  while(*lit)
  {
    // update the current item
    ((SpawnPointListItem*)(*lit))->update();

    // iterate to the next item
    ++lit;
  }

  // make sure the list view is still sorted.
  if (m_keepSorted)
      sortByColumn(sortColumn(), header()->sortIndicatorOrder());

  // iterate over all the spawn points and check how long till they pop
  QHashIterator<QString, SpawnPoint*> it(m_spawnMonitor->spawnPoints());
  SpawnPoint* sp;

  while (it.hasNext())
  {
    it.next();
    sp = it.value();
    if (sp->secsLeft() < 20)
      aboutToPop = true;
  }

  // set timer to update before something is about to pop
  if (aboutToPop)
    m_timer->start(1000L);
  else
    m_timer->start(10000L);
}

void SpawnPointList::newSpawnPoint( const SpawnPoint* sp )
{
  new SpawnPointListItem(this, sp);
}

void SpawnPointList::clear()
{
  SEQListView::clear();
}

SpawnPointListMenu* SpawnPointList::menu()
{
  // if a menu already exists return it
  if (m_menu != NULL)
    return m_menu;
  
  // create a new menu
  m_menu = new SpawnPointListMenu(this, this, "spawnlist menu");

  return m_menu;
}


SpawnPointListMenu::SpawnPointListMenu(SpawnPointList* spawnPointList,
				       QWidget* parent, const char* name)
  : QMenu(parent),
    m_spawnPointList(spawnPointList),
    m_currentItem(NULL)
{
  this->setObjectName(name);
  m_action_rename = addAction("&Rename Spawn Point...",
          this, SLOT(rename_item()));
  m_action_delete = addAction("&Delete Spawn Point...",
          this, SLOT(delete_item()));
  addAction("&Clear Spawn Points...",
          m_spawnPointList, SLOT(clearItems(void)));

  QMenu* listColMenu = new QMenu("Show &Column");
  QAction* listColMenuAction = addMenu(listColMenu);
  listColMenuAction->setCheckable(true);

  m_action_cols[tSpawnPointCoord1] = listColMenu->addAction("Coord &1");
  m_action_cols[tSpawnPointCoord1]->setCheckable(true);
  m_action_cols[tSpawnPointCoord1]->setData(tSpawnPointCoord1);

  m_action_cols[tSpawnPointCoord2] = listColMenu->addAction("Coord &2");
  m_action_cols[tSpawnPointCoord2]->setCheckable(true);
  m_action_cols[tSpawnPointCoord2]->setData(tSpawnPointCoord2);

  m_action_cols[tSpawnPointCoord3] = listColMenu->addAction("Coord &3");
  m_action_cols[tSpawnPointCoord3]->setCheckable(true);
  m_action_cols[tSpawnPointCoord3]->setData(tSpawnPointCoord3);

  m_action_cols[tSpawnPointRemaining] = listColMenu->addAction("&Remaining");
  m_action_cols[tSpawnPointRemaining]->setCheckable(true);
  m_action_cols[tSpawnPointRemaining]->setData(tSpawnPointRemaining);

  m_action_cols[tSpawnPointName] = listColMenu->addAction("&Name");
  m_action_cols[tSpawnPointName]->setCheckable(true);
  m_action_cols[tSpawnPointName]->setData(tSpawnPointName);

  m_action_cols[tSpawnPointLast] = listColMenu->addAction("&Last");
  m_action_cols[tSpawnPointLast]->setCheckable(true);
  m_action_cols[tSpawnPointLast]->setData(tSpawnPointLast);

  m_action_cols[tSpawnPointSpawned] = listColMenu->addAction("&Spawned");
  m_action_cols[tSpawnPointSpawned]->setCheckable(true);
  m_action_cols[tSpawnPointSpawned]->setData(tSpawnPointSpawned);

  m_action_cols[tSpawnPointCount] = listColMenu->addAction("&Count");
  m_action_cols[tSpawnPointCount]->setCheckable(true);
  m_action_cols[tSpawnPointCount]->setData(tSpawnPointCount);

  connect (listColMenu, SIGNAL(triggered(QAction*)),
          this, SLOT(toggle_col(QAction*)));

  addSeparator();
  addAction("&Font...", this, SLOT(set_font()));
  addAction("&Caption...", this, SLOT(set_caption()));

  addSeparator();

  m_action_keepSorted = addAction("Keep Sorted", this, SLOT(toggle_keepSorted()));
  m_action_keepSorted->setCheckable(true);
  m_action_keepSorted->setChecked(m_spawnPointList->keepSorted());

  connect(this, SIGNAL(aboutToShow()),
	  this, SLOT(init_menu()));
}

SpawnPointListMenu::~SpawnPointListMenu()
{
}

void SpawnPointListMenu::setCurrentItem(const SpawnPointListItem* item)
{
  m_currentItem = item;
  m_action_rename->setEnabled(m_currentItem != NULL);
  m_action_delete->setEnabled(m_currentItem != NULL);
}

void SpawnPointListMenu::init_menu()
{
  m_action_rename->setEnabled(m_currentItem != NULL);
  m_action_delete->setEnabled(m_currentItem != NULL);

  // make sure the menu bar settings are correct
  for (int i = 0; i < tSpawnPointMaxCols; i++)
      m_action_cols[i]->setChecked(m_spawnPointList->columnVisible(i));
}

void SpawnPointListMenu::rename_item()
{
  m_spawnPointList->renameItem(m_currentItem);
}

void SpawnPointListMenu::delete_item()
{
  m_spawnPointList->deleteItem(m_currentItem);
}

void SpawnPointListMenu::toggle_col(QAction* col)
{
  int colnum = col->data().value<int>();
  m_spawnPointList->setColumnVisible(colnum, col->isChecked());
}

void SpawnPointListMenu::set_font()
{
  QFont newFont;
  bool ok = false;
  SEQWindow* window = (SEQWindow*)m_spawnPointList->parent();

  // get a new font
  newFont = QFontDialog::getFont(&ok, window->font(), 
				 this, QString("ShowEQ Spawn Point List Font"));
    
    
    // if the user entered a font and clicked ok, set the windows font
    if (ok)
      window->setWindowFont(newFont);
}

void SpawnPointListMenu::set_caption()
{
  bool ok = false;
  SEQWindow* window = (SEQWindow*)m_spawnPointList->parent();

  QString caption =
    QInputDialog::getText(this, "ShowEQ Spawn Point Window Caption",
            "Enter caption for the Spawn Point Window:",
            QLineEdit::Normal, window->windowTitle(), &ok);

  // if the user entered a caption and clicked ok, set the windows caption
  if (ok)
    window->setCaption(caption);
}


void SpawnPointListMenu::toggle_keepSorted()
{
  // toggle immediate update value
  m_spawnPointList->setKeepSorted(!m_spawnPointList->keepSorted());
  m_action_keepSorted->setChecked(m_spawnPointList->keepSorted());
}

SpawnPointWindow::SpawnPointWindow(SpawnMonitor* spawnMonitor, 
				   QWidget* parent, const char* name):
        SEQWindow("SpawnPointList", "ShowEQ - Spawn Points", parent, name)
{
  //QVBoxLayout*    layout = new QVBoxLayout( this );
  //layout->setAutoAdd( true );
  
  m_spawnPointList = new SpawnPointList(spawnMonitor, this, name );
  setWidget(m_spawnPointList);
};

SpawnPointWindow::~SpawnPointWindow()
{
}

QMenu* SpawnPointWindow::menu()
{
  // retrieve the spawn point list menu
  SpawnPointListMenu* splMenu = m_spawnPointList->menu();
  
  // since being brought up without an item, set a 0 current item
  splMenu->setCurrentItem(0);

  // return the menu
  return (QMenu*)splMenu;
}

void SpawnPointWindow::savePrefs(void)
{
  // make base class save it's preferences
  SEQWindow::savePrefs();

  // make the spawn point listview save it's preferences
  if (m_spawnPointList)
    m_spawnPointList->savePrefs();
}

#ifndef QMAKEBUILD
#include "spawnpointlist.moc"
#endif

