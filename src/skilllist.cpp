/*
 *  skilllist.cpp
 *  Copyright 2001-2007, 2019 by the respective ShowEQ Developers
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

#include "player.h"
#include "skilllist.h"
#include "util.h"
#include "main.h" // for pSEQPrefs & showeq_params
#include "diagnosticmessages.h"

#include <QLayout>


SkillList::SkillList(Player* player,
                     QWidget* parent,
                     const char* name)
    : SEQListView("SkillList", parent, name),
    m_pPlayer(player)
{
  int i;

  // initialize the lists
  for (i = 0; i < MAX_KNOWN_SKILLS; i++)
    m_skillList[i] = NULL;

  for (i = 0; i < MAX_KNOWN_LANGS; i++)
    m_languageList[i] = NULL;

  // add the columns
  addColumn("Skill");
  addColumn("Value");

  restoreColumns();

  // connect to player signals
   connect (m_pPlayer, SIGNAL(addSkill(int,int)), 
	    this, SLOT(addSkill(int,int)));
   connect (m_pPlayer, SIGNAL(changeSkill(int,int)), 
	    this, SLOT(changeSkill(int,int)));
   connect (m_pPlayer, SIGNAL(deleteSkills()), 
	    this, SLOT(deleteSkills()));
   connect (m_pPlayer, SIGNAL(addLanguage(int,int)), 
	    this, SLOT(addLanguage(int,int)));
   connect (m_pPlayer, SIGNAL(changeLanguage(int,int)), 
	    this, SLOT(changeLanguage(int,int)));
   connect (m_pPlayer, SIGNAL(deleteLanguages()), 
	    this, SLOT(deleteLanguages()));

   for (i = 0; i < MAX_KNOWN_SKILLS; i++)
     addSkill(i, m_pPlayer->getSkill(i));

   // show the languages or not according to the user preference
   m_showLanguages = pSEQPrefs->getPrefBool("ShowLanguages", preferenceName(),
					    true);
   if (m_showLanguages)
     addLanguages();
}

SkillList::~SkillList()
{
}

/* Called to add a skill to the skills list */
void SkillList::addSkill (int skillId, int value)
{
  if (skillId >= MAX_KNOWN_SKILLS)
  {
    seqWarn("skillId (%d) is more than max skillId (%d)\n", 
	   skillId, MAX_KNOWN_SKILLS - 1);

    return;
  }

  // Purple: Skills are uint32_now, but these special values don't seem to have
  //         been moved up to the top bits. Somehow, the client still knows
  //         the difference between a skill you don't get get and a skill that
  //         you do get and can train and should be shown in the list. For us,
  //         for now all skills show up and are skill 0 whether you can learn
  //         them or now.
#if 0
  if (value == 255)
    return;

  QString str;

  /* Check if this is a skill not learned yet */
  if (value == 254)
    str = " NA";
  else
    str.sprintf ("%3d", value);
#endif
  QString str;

#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  str = QString::asprintf("%3d", value);
#else
  str.sprintf("%3d", value);
#endif

  /* If the skill is not added yet, look up the correct skill namd and add it
   * to the list
   */
  if (!m_skillList[skillId])
  {
      QStringList item_values;
      item_values << skill_name(skillId) << str;
      m_skillList[skillId] = new SEQListViewItem (this, item_values);
  }
  else
    m_skillList[skillId]->setText (1, str);
}

/* Skill update */
void SkillList::changeSkill (int skillId, int value)
{
  if (skillId >= MAX_KNOWN_SKILLS)
  {
    seqWarn("skillId (%d) is more than max skillId (%d)\n",
            skillId, MAX_KNOWN_SKILLS - 1);

    return;
  }

  QString str;
  /* Update skill value with new value */
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  str = QString::asprintf ("%3d", value);
#else
  str.sprintf ("%3d", value);
#endif

  // create skill entry if needed or set the value of the existing item
  if (!m_skillList[skillId])
  {
      QStringList item_values;
      item_values << skill_name(skillId) << str;
      m_skillList[skillId] = new SEQListViewItem (this, item_values);
  }
  else
    m_skillList[skillId]->setText (1, str);
}

/* Delete all skills when zoning */
void SkillList::deleteSkills() 
{
  for(int i=0; i < MAX_KNOWN_SKILLS; i++)
    if(m_skillList[i] != NULL) 
    {
      delete m_skillList[i];
      m_skillList[i] = NULL;
    }
}

// Called to add a language to the skills list
void SkillList::addLanguage (int langId, int value)
{
  // only add it if languages are being shown
  if (!m_showLanguages)
    return;

  if (langId >= MAX_KNOWN_LANGS)
  {
    seqWarn("langId (%d) is more than max langId (%d)\n", 
	   langId, MAX_KNOWN_LANGS - 1);

    return;
  }

  // Check if this is a valid skill 
  if (value == 255)
    return;

  QString str;

  // Check if this is a skill not learned yet
  if (value == 00)
    str = " NA";
  else
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    str = QString::asprintf ("%3d", value);
#else
    str.sprintf ("%3d", value);
#endif

  // If the language is not added yet, look up the correct skill namd and 
  // add it to the list
  if (!m_languageList[langId])
  {
      QStringList item_values;
      item_values << language_name(langId) << str;
      m_languageList[langId] = new SEQListViewItem (this, item_values);
  }
  else
      m_languageList[langId]->setText (1, str);
}

/* Language update */
void SkillList::changeLanguage (int langId, int value)
{
  // only change it if languages are being shown
  if (!m_showLanguages)
    return;

  if (langId > MAX_KNOWN_LANGS)
  {
    seqWarn("Warning: langId (%d) is more than max langId (%d)\n", 
	    langId, MAX_KNOWN_LANGS - 1);

    return;
  }

  QString str;

  /* Update skill value with new value */
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  str = QString::asprintf ("%3d", value);
#else
  str.sprintf ("%3d", value);
#endif

  // create language entry if needed or set the value of the existing item
  if (!m_languageList[langId])
  {
      QStringList item_values;
      item_values << language_name(langId) << str;
      m_languageList[langId] = new SEQListViewItem (this, item_values);
  }
  else
    m_languageList[langId]->setText (1, str);
}

/* Delete all skills when zoning */
void SkillList::deleteLanguages() 
{
  for(int i=0; i < MAX_KNOWN_LANGS; i++)
    if(m_languageList[i] != NULL) 
    {
      delete m_languageList[i];
      m_languageList[i] = NULL;
    }
}

void SkillList::addLanguages() 
{
  if (!m_showLanguages)
    return;

  for (int i = 0; i < MAX_KNOWN_LANGS; i++)
    addLanguage(i, m_pPlayer->getLanguage(i));
}

void SkillList::showLanguages(bool show)
{
  m_showLanguages = show;

  // only save language visibility if the user has set for it
  if (pSEQPrefs->getPrefBool("SaveShowLanguages", preferenceName(), true))
    pSEQPrefs->setPrefBool("ShowLanguages", preferenceName(), m_showLanguages);

  if (m_showLanguages)
    addLanguages();
  else
    deleteLanguages();
}

SkillListWindow::SkillListWindow(Player* player, 
				 QWidget* parent, const char* name)
  : SEQWindow("SkillList", "ShowEQ - Skills", parent, name)
{
  //  QVBoxLayout* layout = new QVBoxLayout(this);
  //layout->setAutoAdd(true);
  
  m_skillList = new SkillList(player, this, name);
  setWidget(m_skillList);
}

SkillListWindow::~SkillListWindow()
{
  delete m_skillList;
}

void SkillListWindow::savePrefs(void)
{
  // save SEQWindow prefs
  SEQWindow::savePrefs();

  // make the listview save it's prefs
  m_skillList->savePrefs();
}

#ifndef QMAKEBUILD
#include "skilllist.moc"
#endif

