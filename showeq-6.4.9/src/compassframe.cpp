/*
 *  compassframe.cpp
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

#include <QFont>
#include <QLayout>
#include <QVBoxLayout>
#include <QLabel>

#include "main.h"
#include "compassframe.h"

CompassFrame::CompassFrame(Player* player, QWidget* parent, const char* name)
  : SEQWindow("Compass", "ShowEQ - Compass", parent, name)
{

  QWidget* mainWidget = new QWidget();
  setWidget(mainWidget);

  QVBoxLayout* layout = new QVBoxLayout(mainWidget);
  layout->setContentsMargins(0, 0, 0, 0);

  m_compass = new Compass (this, "compass");
  layout->addWidget(m_compass);
  QWidget* coordsbox = new QWidget(this);
  QHBoxLayout* coordsboxLayout = new QHBoxLayout(coordsbox);
  layout->addWidget(coordsbox);
  m_compass->setFixedWidth(120);
  m_compass->setFixedHeight(120);

  for(int a=0;a<2;a++) 
  {
    if((a+showeq_params->retarded_coords)%2 == 0) 
    {
      // Create the x: label
      QLabel *labelx = new QLabel(showeq_params->retarded_coords?"E/W:":"X:",
              this);
      labelx->setFixedHeight(labelx->sizeHint().height());
      labelx->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
      coordsboxLayout->addWidget(labelx);

      // Create the xpos label
      m_x = new QLabel("----", this);
      m_x->setFixedHeight(m_x->sizeHint().height());
      m_x->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      coordsboxLayout->addWidget(m_x);
    }
    else
    {
      // Create the y: label
      QLabel *labely = new QLabel(showeq_params->retarded_coords?"N/S:":"Y:",
              this);
      labely->setFixedHeight(labely->sizeHint().height());
      labely->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
      coordsboxLayout->addWidget(labely);

      // Create the ypos label
      m_y = new QLabel("----", this);
      m_y->setFixedHeight(m_y->sizeHint().height());
      m_y->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      coordsboxLayout->addWidget(m_y);
    }
   }

  // Create the z: label
  QLabel *labelz = new QLabel("Z:", this);
  labelz->setFixedHeight(labelz->sizeHint().height());
  labelz->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  coordsboxLayout->addWidget(labelz);

  // Create the zpos label
  m_z = new QLabel("----", this);
  m_z->setFixedHeight(m_z->sizeHint().height());
  m_z->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  coordsboxLayout->addWidget(m_z);

  // connect
  connect(player, SIGNAL(posChanged(int16_t,int16_t,int16_t,
				    int16_t,int16_t,int16_t,int32_t)), 
	  this, SLOT(posChanged(int16_t,int16_t,int16_t,
				int16_t,int16_t,int16_t,int32_t)));

  // initialize compass
  m_compass->setPos(player->x(), player->y(), player->z());
  m_compass->setHeading(player->headingDegrees());
}

CompassFrame::~CompassFrame()
{
}

void CompassFrame::selectSpawn(const Item* item)
{
   if (item)
     m_compass->setTargetPos(item->x(), item->y(), item->z());
   else
     m_compass->clearTarget();
}

void CompassFrame::posChanged(int16_t x, int16_t y, int16_t z,
			      int16_t deltaX, int16_t deltaY, int16_t deltaZ,
			      int32_t heading)
{
  // set compass info
  m_compass->setPos(x, y, z);
  m_compass->setHeading(heading);

  // set position labels
  m_x->setText(QString::number(x));
  m_y->setText(QString::number(y));
  m_z->setText(QString::number(z));
}

#ifndef QMAKEBUILD
#include "compassframe.moc"
#endif

