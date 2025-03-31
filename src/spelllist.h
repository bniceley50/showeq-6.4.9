/*
 *  spelllist.h
 *  Copyright 2001 Crazy Joe Divola (cjd1@users.sourceforge.net)
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
 * Orig Author - Crazy Joe Divola (cjd1@users.sourceforge.net)
 * Date - 9/7/2001
 */

#ifndef SPELLLIST_H
#define SPELLLIST_H

#include <QList>
#include <ctime>
#include <sys/time.h>

#include "seqwindow.h"
#include "seqlistview.h"
#include "spellshell.h"
#include "everquest.h"

#define SPELLCOL_SPELLNAME  0
#define SPELLCOL_SPELLID    1
#define SPELLCOL_CASTERID   2
#define SPELLCOL_CASTERNAME 3
#define SPELLCOL_TARGETID   4
#define SPELLCOL_TARGETNAME 5
#define SPELLCOL_CASTTIME   6
#define SPELLCOL_DURATION   7

class SpellListItem : public SEQListViewItem
{
 public:
  SpellListItem(SEQListViewItem *parent);
  SpellListItem(SEQListView *parent = NULL);
  QVariant data(int column, int role) const;
  const QColor textColor();
  void setTextColor(const QColor &color);
  void update();
  void setSpellItem(const SpellItem *);
  const SpellItem* item() const;
  const QString& category() const;
  void setCategory(QString& cat);
 private:
  QColor m_textColor;
  bool m_btextSet;
  const SpellItem *m_item;
  QString m_category;
};

class SpellList : public SEQListView
{
  Q_OBJECT
 public:
  SpellList(SpellShell* sshell, QWidget *parent = 0, const char *name = 0);
  QMenu* menu();
  void SelectItem(const SpellItem *item);
  SpellListItem* Selected();
  SpellListItem* InsertSpell(const SpellItem *item);
  void DeleteItem(const SpellItem *item);
  //SpellItem* AddCategory(QString& name, QColor color = Qt::black);
  //void RemCategory(SpellListItem *);
  //void clearCategories();
  QColor pickSpellColor(const SpellItem *item, QColor def = Qt::black) const;
  //QString& getCategory(SpellListItem *);
  SpellListItem* Find(const SpellItem *);

 signals:
  void listUpdated();   // flags in spawns have changed
  void listChanged();   // categories have changed
  
 public slots:
  // SpellShell signals
  void addSpell(const SpellItem *);
  void delSpell(const SpellItem *);
  void changeSpell(const SpellItem *);
  void selectSpell(const SpellItem *);
  void clear();

  void activated(QAction*);

 protected slots:  
   void init_menu(void);
  void listItemDoubleClicked(QTreeWidgetItem* litem, int col);
  void listMouseRightButtonPressed(QMouseEvent* event);

 private:
  void selectAndOpen(SpellListItem *);
  SpellShell* m_spellShell;
  QList<QString> m_categoryList;
  QList<SpellListItem *> m_spellList;
  QMenu *m_menu;

  QAction* m_action_spellName;
  QAction* m_action_spellId;
  QAction* m_action_casterId;
  QAction* m_action_casterName;
  QAction* m_action_targetId;
  QAction* m_action_targetName;
  QAction* m_action_casttime;
  QAction* m_action_duration;
};

class SpellListWindow : public SEQWindow
{
  Q_OBJECT

 public:
  SpellListWindow(SpellShell* sshell, QWidget* parent = 0, const char* name = 0);
  ~SpellListWindow();
  virtual QMenu* menu();

  SpellList* spellList() { return m_spellList; }

 public slots:
  virtual void savePrefs(void);

 protected:
  SpellList* m_spellList;
};

#endif
