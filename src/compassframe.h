/*
 *  compassframe.h
 *  Copyright 2001-2002, 2019 by the respective ShowEQ Developers
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

#ifndef _COMPASSFRAME_H_
#define _COMPASSFRAME_H_

#include <QLabel>

#include "player.h"
#include "spawnshell.h"
#include "compass.h"
#include "seqwindow.h"

class CompassFrame : public SEQWindow
{
  Q_OBJECT

 public:
  CompassFrame(Player* player, QWidget* parent = 0, const char* name = 0);
  virtual ~CompassFrame();

  Compass* compass() { return m_compass; }

 public slots:
  void selectSpawn(const Item* item);
  void posChanged(int16_t,int16_t,int16_t,int16_t,int16_t,int16_t,int32_t);
  
 private:
  Compass* m_compass;
  QLabel* m_x;
  QLabel* m_y;
  QLabel* m_z;
};

#endif // _COMPASSFRAME_H_
