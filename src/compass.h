/*
 *  compass.h
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

//
// NOTE: Trying to keep this file ShowEQ/Everquest independent to allow it
// to be reused for other Show{} style projects.  Any existing ShowEQ/EQ
// dependencies will be migrated out.
//

#ifndef EQCOMPASS_H
#define EQCOMPASS_H

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif

#include "point.h"

#include <QWidget>
#include <QSize>
#include <QPaintEvent>

///////////////////////////////////////////
// type definitions
typedef Point3D<int16_t> CompassPoint;

class Compass : public QWidget
{
   Q_OBJECT

 public:
   Compass ( QWidget *parent=0, const char *name=0);
   QSize sizeHint() const; // preferred size
   QSizePolicy sizePolicy() const; // size policy

 public slots:
   void setHeading(int32_t degrees);
   void setPos(int16_t x, int16_t y, int16_t z);
   void setTargetPos(int x, int y, int z);
   void clearTarget(void);

 signals:
   void angleChanged (int);

 protected:
   void paintEvent (QPaintEvent *);

 private:
   void paintCompass ( QPainter * );
   void calcTargetHeading();
   QRect compassRect() const;
   int m_ang;
   double m_dSpawnAngle;
   CompassPoint m_cPlayer;
   CompassPoint m_cTarget;
};

#endif // EQCOMPASS_H
