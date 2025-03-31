/*
 *  seqwindow.h
 *  Copyright 2001-2003 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2002-2005, 2019 by the respective ShowEQ Developers
 *
 *  Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 *  for use under the terms of the GNU General Public License,
 *  incorporated herein by reference.
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
 *
 */

#ifndef SEQWINDOW_H
#define SEQWINDOW_H

#include <QWidget>
#include <QString>
#include <QDockWidget>
#include <QMenu>
#include <QMouseEvent>

class QMenu;

class SEQWindow : public QDockWidget
{
   Q_OBJECT

 public:
   SEQWindow(const QString prefName, const QString caption,
	    QWidget* parent = 0, const char* name = 0, Qt::WindowFlags f = Qt::Widget);
  ~SEQWindow();

  virtual QMenu* menu();

  const QString& preferenceName() const { return m_preferenceName; }

 public slots:
   virtual void setCaption(const QString&);
   virtual void setWindowFont(const QFont&);
   virtual void restoreFont();
   virtual void savePrefs(void);
  
   virtual void mousePressEvent(QMouseEvent* e);

 private:
  QString m_preferenceName;
};

#endif // SEQWINDOW_H

