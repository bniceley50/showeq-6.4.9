/*
 *  skilllist.h
 *  Copyright 2001-2003, 2019 by the respective ShowEQ Developers
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

#ifndef EQSKILLLIST_H
#define EQSKILLLIST_H

#include <QWidget>

#include "seqlistview.h"
#include "seqwindow.h"
#include "player.h"

#define SKILLCOL_NAME 0
#define SKILLCOL_VALUE 1

class SkillList : public SEQListView
{
   Q_OBJECT

 public:
   // constructor
   SkillList (Player* player,
              QWidget*  parent = 0,
              const char* name = 0);

   // destructor
   ~SkillList();

   bool showLanguages() { return m_showLanguages; }

 public slots:
   void addSkill(int skillId, int value);
   void changeSkill(int skillId, int value);
   void deleteSkills(void);
   void addLanguage(int langId, int value);
   void changeLanguage(int langId, int value);
   void deleteLanguages(void);
   void addLanguages(void);
   void showLanguages(bool show);

 private:
   // the player this skill list is monitoring
   Player* m_pPlayer;

   // the list view items related to skills
   SEQListViewItem* m_skillList[MAX_KNOWN_SKILLS];

   // the list view items related to languages
   SEQListViewItem* m_languageList[MAX_KNOWN_LANGS];

   // whether or not to show languages
   bool m_showLanguages;
};

class SkillListWindow : public SEQWindow
{
  Q_OBJECT

 public:
  SkillListWindow(Player* player, QWidget* parent = 0, const char* name = 0);
  ~SkillListWindow();
  SkillList* skillList() { return m_skillList; }

 public slots:
  virtual void savePrefs(void);

 protected:
  SkillList* m_skillList;
};

#endif // EQSKILLLIST_H
