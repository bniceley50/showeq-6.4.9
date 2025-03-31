/*
 *  guildlist.cpp
 *  Copyright 2004-2007 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2004-2007, 2019 by the respective ShowEQ Developers
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

#include "guildlist.h"
#include "guildshell.h"
#include "player.h"
#include "zonemgr.h"
#include "main.h"

#include <QFont>
#include <QPainter>
#include <QFontDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>

//----------------------------------------------------------------------
// GuildListItem
GuildListItem::GuildListItem(SEQListView* parent,
                             const GuildMember* member,
                             const GuildShell* guildShell)
  : SEQListViewItem(parent),
    m_member(member)
{
  update(guildShell);
}

GuildListItem::~GuildListItem()
{
}

QVariant GuildListItem::data(int column, int role) const
{
    QFont font = treeWidget()->font();
    switch(role)
    {
        case Qt::FontRole:
            if (m_member->zoneId())
                font.setBold(true);
            else
                font.setBold(false);

            return font;

        default:
            return SEQListViewItem::data(column, role);
    }

}

bool GuildListItem::operator<(const GuildListItem& other) const
{
    int column = treeWidget() ? treeWidget()->sortColumn() : 0;

    switch(column)
    {
        case 1: // level
            return data(column, Qt::DisplayRole).value<int>() <
                other.data(column, Qt::DisplayRole).value<int>();

        case 6: // last on
            return guildMember()->lastOn() < other.guildMember()->lastOn();

        case 0: // name
        case 2: // class
        case 3: // rank
        case 4: // banker
        case 5: // alt
        case 7: // zone
        case 8: // public note
        default: // Qt sorts values as strings by default
            return text(column) < other.text(column);
    }
}

static const QString dateFormat("ddd MMM dd hh:mm:ss yyyy");

void GuildListItem::update(const GuildShell* guildShell)
{
  if (!m_member)
    return;

  setText(tGuildListColName, m_member->name());
  setText(tGuildListColLevel, QString::number(m_member->level()));
  setText(tGuildListColClass, m_member->classString());
  setText(tGuildListColRank, m_member->guildRankString());
  setText(tGuildListColBank, m_member->bankRankString());
  setText(tGuildListColAlt, m_member->altRankString());
  QDateTime dt;
  dt.setTime_t(m_member->lastOn());
  setText(tGuildListColLastOn, dt.toString(dateFormat));
  QString zoneString = guildShell->zoneString(m_member->zoneId());
  if (m_member->zoneInstance())
    zoneString += ":" + QString::number(m_member->zoneInstance());
  setText(tGuildListColZone, zoneString);
  setText(tGuildListColPublicNote, m_member->publicNote());
}

void GuildListItem::setGuildMember(const GuildMember* member)
{
  m_member = member;
}

int GuildListItem::rtti() const
{
  return 2004;
}

//----------------------------------------------------------------------
// GuildListWindow
GuildListWindow::GuildListWindow(Player* player,
				 GuildShell* guildShell,
				 QWidget* parent, const char* name)
  : SEQWindow("GuildList", "ShowEQ - Guild Member List", parent, name),
    m_player(player),
    m_guildShell(guildShell),
    m_guildListItemDict(),
    m_menu(0),
    m_membersOn(0)
{
  // get whether to show offline guildmates or not
  m_showOffline = pSEQPrefs->getPrefBool("ShowOffline", preferenceName(),
					 false);

  // get whether to keep the list sorted or not
  m_keepSorted = pSEQPrefs->getPrefBool("KeepSorted", preferenceName(), false);

  m_showAlts = pSEQPrefs->getPrefBool("ShowAlts", preferenceName(), true);

  QWidget* mainWidget = new QWidget();
  setWidget(mainWidget);

  QBoxLayout* vLayout = new QVBoxLayout(mainWidget);
  vLayout->setContentsMargins(0, 0, 0, 0);
  QHBoxLayout* hLayout= new QHBoxLayout();
  vLayout->addLayout(hLayout);

  // Guild Name
  m_guildName = new QLabel("Guild", this);
  m_guildName->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  m_guildName->setFrameShape(QFrame::StyledPanel);
  m_guildName->setFrameShadow(QFrame::Sunken);
  m_guildName->setMinimumWidth(50);
  m_guildName->setMaximumWidth(300);
  hLayout->addWidget(m_guildName, 1, Qt::AlignLeft);
  guildChanged();

  // Guild Totals
  m_guildTotals = new QLabel("", this);
  m_guildTotals->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  m_guildTotals->setFrameShape(QFrame::StyledPanel);
  m_guildTotals->setFrameShadow(QFrame::Sunken);
  m_guildTotals->setMinimumWidth(30);
  m_guildTotals->setMaximumWidth(120);
  hLayout->addWidget(m_guildTotals, 0, Qt::AlignRight);

  // create the spawn listview
  m_guildList = new SEQListView(preferenceName(), this, "guildlistview");
  m_guildList->setSortingEnabled(m_keepSorted);

  vLayout->addWidget(m_guildList);

  // setup the columns
  m_guildList->addColumn("Name");
  m_guildList->addColumn("Level");
  m_guildList->addColumn("Class");
  m_guildList->addColumn("Rank");
  m_guildList->addColumn("Banker");
  m_guildList->addColumn("Alt");
  m_guildList->addColumn("Last On", "LastOn");
  m_guildList->addColumn("Zone");
  m_guildList->addColumn("Public Note", "PublicNote");

  // restore the columns settings from preferences
  m_guildList->restoreColumns();

  connect(m_guildShell, SIGNAL(cleared()),
	  this, SLOT(cleared()));
  connect(m_guildShell, SIGNAL(loaded()),
	  this, SLOT(loaded()));
  connect(m_guildShell, SIGNAL(updated(const GuildMember*)),
	  this, SLOT(updated(const GuildMember*)));

  connect(m_player, SIGNAL(guildChanged()),
	  this, SLOT(guildChanged()));

  connect(m_guildList, SIGNAL(mouseRightButtonPressed(QMouseEvent*)),
          this, SLOT(listMouseRightButtonPressed(QMouseEvent*)));

  // populate the window
  populate();
}

GuildListWindow::~GuildListWindow()
{
}

void GuildListWindow::listMouseRightButtonPressed(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        menu()->popup(event->globalPos());
    }
}

QMenu* GuildListWindow::menu()
{
  if (m_menu)
    return m_menu;

  m_menu = new QMenu;
  QMenu* guildListColMenu = new QMenu("Show &Column");
  m_menu->addMenu(guildListColMenu);

  m_action_guildList_Cols[tGuildListColName] = guildListColMenu->addAction("&Name");
  m_action_guildList_Cols[tGuildListColName]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColName]->setData(tGuildListColName);

  m_action_guildList_Cols[tGuildListColLevel] = guildListColMenu->addAction("&Level");
  m_action_guildList_Cols[tGuildListColLevel]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColLevel]->setData(tGuildListColLevel);

  m_action_guildList_Cols[tGuildListColClass] = guildListColMenu->addAction("&Class");
  m_action_guildList_Cols[tGuildListColClass]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColClass]->setData(tGuildListColClass);

  m_action_guildList_Cols[tGuildListColRank] = guildListColMenu->addAction("&Rank");
  m_action_guildList_Cols[tGuildListColRank]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColRank]->setData(tGuildListColRank);

  m_action_guildList_Cols[tGuildListColBank] = guildListColMenu->addAction("&Banker");
  m_action_guildList_Cols[tGuildListColBank]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColBank]->setData(tGuildListColBank);

  m_action_guildList_Cols[tGuildListColAlt] = guildListColMenu->addAction("&Alt");
  m_action_guildList_Cols[tGuildListColAlt]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColAlt]->setData(tGuildListColAlt);

  m_action_guildList_Cols[tGuildListColLastOn] = guildListColMenu->addAction("Last &On");
  m_action_guildList_Cols[tGuildListColLastOn]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColLastOn]->setData(tGuildListColLastOn);

  m_action_guildList_Cols[tGuildListColZone] = guildListColMenu->addAction("&Zone");
  m_action_guildList_Cols[tGuildListColZone]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColZone]->setData(tGuildListColZone);

  m_action_guildList_Cols[tGuildListColPublicNote] = guildListColMenu->addAction("&Public Note");
  m_action_guildList_Cols[tGuildListColPublicNote]->setCheckable(true);
  m_action_guildList_Cols[tGuildListColPublicNote]->setData(tGuildListColPublicNote);

  connect (guildListColMenu, SIGNAL(triggered(QAction*)), this,
          SLOT(toggle_guildListCol(QAction*)));

  m_menu->addSeparator();
  QAction* tmpAction;

  tmpAction = m_menu->addAction("Show Offline", this, SLOT(toggle_showOffline(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_showOffline);

  tmpAction = m_menu->addAction("Show Alts", this, SLOT(toggle_showAlts(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_showAlts);

  tmpAction = m_menu->addAction("Keep Sorted", this, SLOT(toggle_keepSorted(bool)));
  tmpAction->setCheckable(true);
  tmpAction->setChecked(m_keepSorted);

  m_menu->addSeparator();
  m_menu->addAction("&Font...", this, SLOT(set_font()));
  m_menu->addAction("&Caption...", this, SLOT(set_caption()));

  connect(m_menu, SIGNAL(aboutToShow()), this, SLOT(init_Menu()));

  return m_menu;
}

void GuildListWindow::cleared()
{
  clear();
}

void GuildListWindow::loaded()
{
  populate();
}
 
void GuildListWindow::updated(const GuildMember* member)
{
  GuildListItem* memberItem = m_guildListItemDict.value((void*)member, nullptr);

  if (memberItem)
  {
    // We have them in our list already. Need to update.
    bool bRemove = false;

    if (! m_showAlts && member->altRank())
    {
        // This is an alt and we're not showing alts
        bRemove = true;
    }

    // if not-showing offline users and this user has become offline,
    // then remove it
    if (! m_showOffline && ! member->zoneId())
    {
        // This dude is offline and we're not showing offline.
        bRemove = true;
    }

    // If we got an update for someone we had, but now they are offline,
    // make them offline
    if (! member->zoneId())
    {
        m_membersOn--;
    }

    if (bRemove)
    {
        // remove the item from the item dictionary
        delete m_guildListItemDict.take((void*)member);
    }
    else
    {
        memberItem->update(m_guildShell);
    }
  }
  else 
  {
    // Not in list yet.
    if (member->zoneId())
    {
        // Online.
        m_membersOn++;
    }

    // Assume we should add them.
    bool bAdd = true;

    // Don't add ignored offliners.
    if (! m_showOffline && ! member->zoneId())
    {
        bAdd = false;
    }

    // Don't add ignored alts.
    if (! m_showAlts && member->altRank())
    {
        bAdd = false;
    }

    if (bAdd)
    {
        // add the new guild member item
        memberItem = new GuildListItem(m_guildList, member, m_guildShell);
      
        // insert it into the dictionary
        m_guildListItemDict.insert((void*)member, memberItem);
    }
  }

  // make sure the guild list is sorted
  if (m_keepSorted)
      m_guildList->sortByColumn(m_guildList->sortColumn(),
              m_guildList->header()->sortIndicatorOrder());

  updateCount();
}

void GuildListWindow::guildChanged()
{
  QString guild(" Guild: %1 ");

  // set the guild name
  m_guildName->setText(guild.arg(m_player->guildTag()));
}

void GuildListWindow::init_Menu(void)
{
  // make sure the menu bar settings are correct
  for (int i = 0; i < tGuildListColMaxCols; i++)
    m_action_guildList_Cols[i]->setChecked(
            m_guildList->columnVisible(i));
}

void GuildListWindow::toggle_showOffline(bool enable)
{
  m_showOffline = enable;
  pSEQPrefs->setPrefBool("ShowOffline", preferenceName(), 
			 m_showOffline);

  // re-populate the window
  populate();
}

void GuildListWindow::toggle_showAlts(bool enable)
{
  m_showAlts = enable;
  pSEQPrefs->setPrefBool("ShowAlts", preferenceName(), 
			 m_showAlts);

  // re-populate the window
  populate();
}

void GuildListWindow::toggle_keepSorted(bool enable)
{
  m_keepSorted = enable;
  pSEQPrefs->setPrefBool("KeepSorted", preferenceName(), m_keepSorted);
  m_guildList->setSortingEnabled(enable);
  if (m_keepSorted)
      m_guildList->sortByColumn(m_guildList->sortColumn(),
              m_guildList->header()->sortIndicatorOrder());
}

void GuildListWindow::toggle_guildListCol(QAction* col)
{
  int colnum = col->data().value<int>();
  m_guildList->setColumnVisible(colnum, col->isChecked());
}

void GuildListWindow::set_font()
{
  QFont newFont;
  bool ok = false;
  // get a new font
  newFont = QFontDialog::getFont(&ok, font(),
				 this, windowTitle() + " Font");
    
    
    // if the user entered a font and clicked ok, set the windows font
    if (ok)
      setWindowFont(newFont);
}

void GuildListWindow::set_caption()
{
  bool ok = false;

  QString captionText =
    QInputDialog::getText(this, "ShowEQ Guild List Window Caption",
            "Enter caption for the Guild List Window:",
            QLineEdit::Normal, windowTitle());

  // if the user entered a caption and clicked ok, set the windows caption
  if (ok)
    setWindowTitle(captionText);
}

void GuildListWindow::clear(void)
{
  // clear count
  m_membersOn = 0;

  // clear out the guild list item dictionary
  qDeleteAll(m_guildListItemDict);
  m_guildListItemDict.clear();
  
  // clear the guild list contents
  m_guildList->clear();

  updateCount();
}

void GuildListWindow::populate(void)
{
  GuildMember* member;
  GuildListItem* memberItem;

  // make sure everything is out of the list view first...
  clear();

  // disable updates
  setUpdatesEnabled(false);

  // iterate over the members
  GuildMemberDictIterator it(m_guildShell->members());

    // iterate over all the members
    while (it.hasNext())
    {
        it.next();
        member = it.value();
        // increment members on count for each member on
        if (member->zoneId())
        {
	        m_membersOn++;
        }

        bool bAdd = true;

        if (member->altRank() && ! m_showAlts)
        {
            bAdd = false;
        }

        if (! member->zoneId() && ! m_showOffline)
        {
            bAdd = false;
        }

        if (bAdd)
        {
            memberItem = new GuildListItem(m_guildList, member, m_guildShell);
      
            // insert it into the dictionary
            m_guildListItemDict.insert((void*)member, memberItem);
        }

    }

  // make sure the guild list is sorted
  if (m_keepSorted)
      m_guildList->sortByColumn(m_guildList->sortColumn(),
              m_guildList->header()->sortIndicatorOrder());

  // update the counts
  updateCount();

  // re-enable updates and force a repaint
  setUpdatesEnabled(true);
  repaint();
}

void GuildListWindow::updateCount(void)
{
  QString text(" %1 on/%2 total ");
  m_guildTotals->setText(text.arg(m_membersOn).arg(m_guildShell->members().count()));
}

#ifndef QMAKEBUILD
#include "guildlist.moc"
#endif

