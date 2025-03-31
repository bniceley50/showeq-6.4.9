/*
 *  guildlist.h
 *  Copyright 2004 Zaphod (dohpaz@users.sourceforge.net).
 *  Copyright 2006, 2019 by the respective ShowEQ Developers
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

#ifndef _GUILDLIST_H_
#define _GUILDLIST_H_

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif

#include "seqwindow.h"
#include "seqlistview.h"

#include <QHash>
#include <QString>
#include <QLabel>
#include <QMenu>

//----------------------------------------------------------------------
// forward declarations
class Player;
class ZoneMgr;
class GuildMember;
class GuildShell;

class QLabel;
class QLineEdit;
class QMenu;

//--------------------------------------------------
// constants
const int tGuildListColName = 0;
const int tGuildListColLevel = 1;
const int tGuildListColClass = 2;
const int tGuildListColRank = 3;
const int tGuildListColBank = 4;
const int tGuildListColAlt = 5;
const int tGuildListColLastOn = 6;
const int tGuildListColZone = 7;
const int tGuildListColPublicNote = 8;
const int tGuildListColMaxCols = 9;

//----------------------------------------------------------------------
// GuildListItem
class GuildListItem : public SEQListViewItem
{
 public:
   GuildListItem(SEQListView* parent,
                 const GuildMember* member,
                 const GuildShell* guildShell);
   virtual ~GuildListItem();

   bool operator<(const GuildListItem& other) const;

   void update(const GuildShell* guildShell);

   const GuildMember* guildMember() const { return m_member; }
   void setGuildMember(const GuildMember* member);

   QVariant data(int column, int role) const;

   virtual int rtti() const;

 protected:
   const GuildMember* m_member;
};

//----------------------------------------------------------------------
// GuildListWindow
class GuildListWindow : public SEQWindow
{
  Q_OBJECT
    
 public:
  GuildListWindow(Player* player, GuildShell* guildShell,
		  QWidget* parent = 0, const char* name = 0);
  ~GuildListWindow();

  virtual QMenu* menu();

 public slots: 
  void cleared();
  void loaded();
  void updated(const GuildMember* gm);
  void guildChanged();

 protected slots:
  void init_Menu(void);
  void toggle_showOffline(bool enable);
  void toggle_keepSorted(bool enable);
  void toggle_showAlts(bool enable);
  void toggle_guildListCol(QAction* col);
  void set_font();
  void set_caption();
  void listMouseRightButtonPressed(QMouseEvent*);

 protected:
  void clear(void);
  void populate(void);
  void updateCount(void);

  Player* m_player;
  GuildShell* m_guildShell;

  QLabel* m_guildName;
  QLabel* m_guildTotals;
  SEQListView* m_guildList;
  QHash<void*, GuildListItem*> m_guildListItemDict;
  QMenu* m_menu;
  QAction* m_action_guildList_Cols[tGuildListColMaxCols];

  uint32_t m_membersOn;
  bool m_showOffline;
  bool m_showAlts;
  bool m_keepSorted;
};

#endif // _GUILDLIST_H_

