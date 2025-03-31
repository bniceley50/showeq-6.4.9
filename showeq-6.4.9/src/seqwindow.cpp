/*
 *  seqwindow.cpp
 *  Copyright 2001-2003,2007 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2002-2007, 2019 by the respective ShowEQ Developers
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
 */

#include "seqwindow.h"
#include "main.h"

#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>

SEQWindow::SEQWindow(const QString prefName, const QString caption,
		     QWidget* parent, const char* name, Qt::WindowFlags f)
  : QDockWidget(parent),
    m_preferenceName(prefName)
{
  setObjectName(name);
  setWindowFlags(f);
  // set the windows caption
  QDockWidget::setWindowTitle(pSEQPrefs->getPrefString("Caption", preferenceName(),
              caption));

  setFeatures(QDockWidget::DockWidgetClosable |
          QDockWidget::DockWidgetMovable |
          QDockWidget::DockWidgetFloatable);

  // restore the font
  restoreFont();

  // QT4 now insists on putting a QT window icon if we don't specify one,
  // so let's give it one...
  QPixmap pixmap(32, 32);
  pixmap.fill(Qt::transparent);
  setWindowIcon(QIcon(pixmap));
}

SEQWindow::~SEQWindow()
{
}

QMenu* SEQWindow::menu()
{
  return 0;
}

void SEQWindow::setCaption(const QString& text)
{
  // set the caption
  QDockWidget::setWindowTitle(text);
  setObjectName(windowTitle());

  // set the preference
  pSEQPrefs->setPrefString("Caption", preferenceName(), windowTitle());
}

void SEQWindow::setWindowFont(const QFont& font)
{
  // set the font preference
  pSEQPrefs->setPrefFont("Font", preferenceName(), font);

  // restore the font to the preference
  restoreFont();
}

void SEQWindow::restoreFont()
{
  // set the applications default font
  if (pSEQPrefs->isPreference("Font", preferenceName()))
  {
    // use the font specified in the preferences
    QFont font = pSEQPrefs->getPrefFont("Font", preferenceName());
    setFont( font);
  }
}

void SEQWindow::savePrefs(void) {}

void SEQWindow::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::RightButton)
  {
    QMenu* popupMenu = menu();
    if (popupMenu)
    {
      popupMenu->popup(mapToGlobal(e->pos()));
      e->accept();
    }
    else
    {
      QDockWidget::mousePressEvent(e);
    }
  }
  else
    QDockWidget::mousePressEvent(e);
}

#ifndef QMAKEBUILD
#include "seqwindow.moc"
#endif
