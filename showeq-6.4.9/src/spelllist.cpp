/*
 *  spelllist.cpp
 *  Copyright 2001 Crazy Joe Divola (cjd1@users.sourceforge.net)
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
 
/*
 * Orig Author - Crazy Joe Divola (cjd1@users.sourceforge.net)
 * Date - 9/7/2001
 */

#include <QPainter>
#include <QLayout>
#include <QList>
#include <QMenu>

#include "spelllist.h"
#include "main.h"

SpellListItem::SpellListItem(SEQListViewItem *parent) : SEQListViewItem(parent)
{
   m_textColor = Qt::black;
   m_item = NULL;
}

SpellListItem::SpellListItem(SEQListView *parent) : SEQListViewItem(parent)
{
   m_textColor = Qt::black;
   m_item = NULL;
}

QVariant SpellListItem::data(int column, int role) const
{
    switch(role)
    {
        case Qt::ForegroundRole:
            return m_textColor;

        default:
            return SEQListViewItem::data(column, role);
    }
}


const QColor SpellListItem::textColor()
{
   return m_textColor;
}

void SpellListItem::setTextColor(const QColor& color)
{
   m_textColor = color;
}

void SpellListItem::update()
{
   //color change by Worried
   //change spell colors according to time remaining

  if (m_item->duration() > 120)
    this->setTextColor(Qt::black);
  else if (m_item->duration() <= 120 and m_item->duration() > 60)
    this->setTextColor(QColor(128,54,193));
  else if (m_item->duration() <= 60 and m_item->duration() > 30)
    this->setTextColor(Qt::blue);
  else if (m_item->duration() <= 30 and m_item->duration() > 12)
    this->setTextColor(Qt::magenta);
  else if (m_item->duration() <= 12)
    this->setTextColor(Qt::red);

   setText(SPELLCOL_SPELLID, QString("%1").arg(m_item->spellId()));
   setText(SPELLCOL_SPELLNAME, m_item->spellName());
   setText(SPELLCOL_CASTERID, QString("%1").arg(m_item->casterId()));
   setText(SPELLCOL_CASTERNAME, m_item->casterName());
   setText(SPELLCOL_TARGETID, QString("%1").arg(m_item->targetId()));
   setText(SPELLCOL_TARGETNAME, m_item->targetName());
   setText(SPELLCOL_CASTTIME, m_item->castTimeStr());
   setText(SPELLCOL_DURATION, m_item->durationStr());
}

void SpellListItem::setSpellItem(const SpellItem *item)
{
   m_item = item;
}

const SpellItem* SpellListItem::item() const
{
   return m_item;
}

const QString& SpellListItem::category() const
{
   return m_category;
}

void SpellListItem::setCategory(QString& cat)
{
   m_category = cat;
   //CJD TODO - fill in
}

// -------------------------------------------------------------------

SpellList::SpellList(SpellShell* sshell, QWidget *parent, const char *name)
  : SEQListView("SpellList", parent, name),
    m_spellShell(sshell),
    m_menu(0)
{
   //addColumn... spell icon
   addColumn("Spell", "SpellName");
   addColumn("Spell ID", "SpellID");
   addColumn("Caster ID", "CasterID");
   addColumn("Caster", "CasterName");
   addColumn("Target ID", "TargetID");
   addColumn("Target", "TargetName");
   addColumn("Casted", "CastTime");
   addColumn("Remain", "RemainTime");
   setSorting(SPELLCOL_DURATION);

   restoreColumns();

   connect (this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
	    this, SLOT(listItemDoubleClicked(QTreeWidgetItem*, int)));
   connect(this, SIGNAL(mouseRightButtonPressed(QMouseEvent*)),
	   this, SLOT(listMouseRightButtonPressed(QMouseEvent*)));
}

QMenu* SpellList::menu()
{
  // if the menu already exists, return it
  if (m_menu)
    return m_menu;

  m_menu = new QMenu(this);

  m_action_spellName = m_menu->addAction("Spell Name");
  m_action_spellName->setCheckable(true);
  m_action_spellName->setData(SPELLCOL_SPELLNAME);
  m_action_spellId = m_menu->addAction("Spell ID");
  m_action_spellId->setCheckable(true);
  m_action_spellId->setData(SPELLCOL_SPELLID);
  m_action_casterId = m_menu->addAction("Caster ID");
  m_action_casterId->setCheckable(true);
  m_action_casterId->setData(SPELLCOL_CASTERID);
  m_action_casterName = m_menu->addAction("Caster Name");
  m_action_casterName->setCheckable(true);
  m_action_casterName->setData(SPELLCOL_CASTERNAME);
  m_action_targetId = m_menu->addAction("Target ID");
  m_action_targetId->setCheckable(true);
  m_action_targetId->setData(SPELLCOL_TARGETID);
  m_action_targetName = m_menu->addAction("Target Name");
  m_action_targetName->setCheckable(true);
  m_action_targetName->setData(SPELLCOL_TARGETNAME);
  m_action_casttime = m_menu->addAction("Cast Time");
  m_action_casttime->setCheckable(true);
  m_action_casttime->setData(SPELLCOL_CASTTIME);
  m_action_duration = m_menu->addAction("Remaining Time");
  m_action_duration->setCheckable(true);
  m_action_duration->setData(SPELLCOL_DURATION);

  connect(m_menu, SIGNAL(triggered(QAction*)), this, SLOT(activated(QAction*)));
  connect(m_menu, SIGNAL(aboutToShow()), this, SLOT(init_menu()));

  return m_menu;
}

void SpellList::init_menu(void)
{
    foreach (QAction* action, m_menu->actions())
    {
        action->setChecked(columnWidth(action->data().value<int>()) != 0);
    }
}

void SpellList::selectSpell(const SpellItem *item)
{
   if (item) {
      SpellListItem *i = Find(item);
      selectAndOpen(i);
   }
}

SpellListItem* SpellList::InsertSpell(const SpellItem *item)
{
   if (!item)
      return NULL;

   QList<SpellListItem *>::Iterator it;
   for(it = m_spellList.begin(); it != m_spellList.end(); it++) {
      if ((*it)->item() == item)
         break;
   }
   if (it != m_spellList.end()) {
      int sid = (*it)->text(SPELLCOL_SPELLID).toInt();
      int cid = (*it)->text(SPELLCOL_CASTERID).toInt();
      int tid = (*it)->text(SPELLCOL_TARGETID).toInt();
      if ((sid == (*it)->item()->spellId()) &&
          (cid == (*it)->item()->casterId()) &&
          (tid == (*it)->item()->targetId())) {
         (*it)->update();
         return (*it);
      } else {
         DeleteItem((*it)->item());
      }
   }

   // now insert
   // CJD TODO - checks for putting in appropriate category
   SpellListItem *j = new SpellListItem(this);
   m_spellList.append(j);
   j->setSpellItem(item);

   //j->setTextColor(pickColorSpawn(item));
   j->update();

   return j;
}

void SpellList::DeleteItem(const SpellItem *item)
{
   if (item) {
      SpellListItem *i = Find(item);
      if (i) {
         delete m_spellList.takeAt(m_spellList.indexOf(i));
      }
   }
}

//SpellItem* SpellList::AddCategory(QString& name, QColor color)
//{
//}

//void SpellList::RemCategory(SpellListItem *)
//{
//}

//void SpellList::clearCategory()
//{
//}

QColor SpellList::pickSpellColor(const SpellItem *item, QColor def) const
{
   return Qt::black;
}

SpellListItem* SpellList::Find(const SpellItem *item)
{
   if (item) {
      QList<SpellListItem*>::Iterator it;
      for(it = m_spellList.begin(); it != m_spellList.end(); ++it) {
         if ((*it)->item() == item)
            return (*it);
      }
   }
   return NULL;
}

void SpellList::addSpell(const SpellItem *item)
{
   if (item)
      InsertSpell(item);
}

void SpellList::delSpell(const SpellItem *item)
{
   if (item)
      DeleteItem(item);
}

void SpellList::changeSpell(const SpellItem *item)
{
   if (item) {
      SpellListItem *i = Find(item);
      if (!i)
         return;
      int sid = i->text(SPELLCOL_SPELLID).toInt();
      int cid = i->text(SPELLCOL_CASTERID).toInt();
      int tid = i->text(SPELLCOL_TARGETID).toInt();
      if ((sid == item->spellId()) &&
          (cid == item->casterId()) &&
          (tid == item->targetId()))
         i->update();
      else {
         DeleteItem(item);
         addSpell(item);
      }
   }
}

void SpellList::clear()
{
   SEQListView::clear();
   m_spellList.clear();
   // rebuild categories...
}

void SpellList::selectAndOpen(SpellListItem *item)
{
   SEQListViewItem *i = item;
   while(i) {
      expandItem(item);
      item = (SpellListItem *)item->parent();
   }
   setCurrentItem(i);
   // CJD TODO - use keep selected setting?
}

//void selfStartSpellCast(struct castStruct *);
//void otherStartSpellCast(struct beginCastStruct *);
//void selfFinishSpellCast(struct beginCastStruct *);
//void interruptSpellCast(struct interruptCastStruct *);
//void spellMessage(QString&);


void SpellList::listItemDoubleClicked(QTreeWidgetItem* litem, int col)
{
    SpellListItem* lvitem = dynamic_cast<SpellListItem*>(litem);
    if (!lvitem) return;

    const SpellItem *j = lvitem->item();
    if (j)
        m_spellShell->deleteSpell(j);
}


void SpellList::listMouseRightButtonPressed(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        QMenu* slMenu = menu();

        if (slMenu)
            slMenu->popup(event->globalPos());
    }
}

void SpellList::activated(QAction* action)
{
   int col = action->data().value<int>();
   setColumnVisible(col, !columnVisible(col));
}

SpellListWindow::SpellListWindow(SpellShell* sshell, 
				 QWidget* parent, const char* name)
  : SEQWindow("SpellList", "ShowEQ - Spell List", parent, name)
{
  //QVBoxLayout* layout = new QVBoxLayout(this);
  //layout->setAutoAdd(true);
  
  m_spellList = new SpellList(sshell, this, name);
  setWidget(m_spellList);
}

SpellListWindow::~SpellListWindow()
{
  delete m_spellList;
}

QMenu* SpellListWindow::menu()
{
  return m_spellList->menu();
}

void SpellListWindow::savePrefs(void)
{
  // save SEQWindow prefs
  SEQWindow::savePrefs();

  // make the listview save it's prefs
  m_spellList->savePrefs();
}

#ifndef QMAKEBUILD
#include "spelllist.moc"
#endif
