/*
 *  filternotifications.cpp
 *  Copyright 2019 by the respective ShowEQ Developers
 *  Portions Copyright 2003 Zaphod (dohpaz@users.sourceforge.net).
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

#ifndef _FILTERNOTIFICATIONS_H_
#define _FILTERNOTIFICATIONS_H_

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif

#include <QObject>

class QString;

class Item;

class FilterNotifications : public QObject
{
  Q_OBJECT
 public:
  FilterNotifications(QObject* parent = 0, const char* name = 0);
  ~FilterNotifications();
  
  bool useSystemBeep() const { return m_useSystemBeep; }
  bool useCommands() const { return m_useCommands; }

 public slots:
   void setUseSystemBeep(bool val);
   void setUseCommands(bool val);
   
   void addItem(const Item* item);
   void delItem(const Item* item);
   void killSpawn(const Item* item);
   void changeItem(const Item* item, uint32_t changeType);

 protected:
   void handleAlert(const Item* item, 
		    const QString& commandPref, const QString& cue);
   void beep(void);
   void executeCommand(const Item* item, 
		       const QString& rawCommand,
		       const QString& audioCue);

 protected:
  bool m_useSystemBeep;
  bool m_useCommands;
};

#endif // _FILTERNOTIFICATIONS_H_

