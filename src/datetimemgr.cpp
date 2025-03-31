/*
 *  datetimemgr.cpp
 *  Copyright 2003, 2007 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *  Copyright 2019 by the respective ShowEQ Developers
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
#include <datetimemgr.h>
#include "everquest.h"

#include <QDateTime>
#include <QTimer>

DateTimeMgr::DateTimeMgr(QObject* parent, const char* name)
  : QObject(parent),
    m_updateFrequency(60 * 1000),
    m_timer(0)
{
    setObjectName(name);
}

void DateTimeMgr::setUpdateFrequency(int seconds)
{
  // set the new frequency (in ms)
  m_updateFrequency = seconds * 1000;

  if (m_timer)
  {
    // update the current time
    update();

    // set the timer to the new interval
    m_timer->setInterval(m_updateFrequency);
  }
}

void DateTimeMgr::timeOfDay(const uint8_t* data)
{
  const timeOfDayStruct* tday = (const timeOfDayStruct*)data;

  m_refDateTime = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);
  m_eqDateTime.setDate(QDate(tday->year, tday->month, tday->day));
  m_eqDateTime.setTime(QTime(tday->hour - 1, tday->minute, 0));
  if (!m_timer)
  {
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), SLOT(update()));
    m_timer->start(m_updateFrequency);
  }

  emit syncDateTime(m_eqDateTime);
}

void DateTimeMgr::update()
{
  if (!m_eqDateTime.isValid())
    return;

  const QDateTime& current = QDateTime::currentDateTime().toTimeSpec(Qt::UTC);

  int secs = m_refDateTime.secsTo(current);
  if (secs)
  {
    m_eqDateTime = m_eqDateTime.addSecs(secs * 20);
    m_refDateTime = current;
    emit updatedDateTime(m_eqDateTime);
  }
}

#ifndef QMAKEBUILD
#include "datetimemgr.moc"
#endif

