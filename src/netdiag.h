/*
 *  netdiag.h
 *  Copyright 2002-2005, 2019 by the respective ShowEQ Developers
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

#ifndef EQNETDIAG_H
#define EQNETDIAG_H

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <netinet/in.h>

#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QSpinBox>

#include "packetcommon.h"
#include "seqwindow.h"

//----------------------------------------------------------------------
// forward declarations
class EQPacket;

//----------------------------------------------------------------------
// NetDiag window class
class NetDiag : public SEQWindow
{
  Q_OBJECT 
 public:
  NetDiag(EQPacket* packet, QWidget* parent, const char* name);
  ~NetDiag();

 public slots:
   void numPacket              (int, int);
   void resetPacket            (int, int);
   void clientChanged          (in_addr_t);
   void clientPortLatched      (in_port_t);
   void serverPortLatched      (in_port_t);
   void sessionTrackingChanged (uint8_t);
   void filterChanged          ();
   void seqReceive             (int, int);
   void seqExpect              (int, int);
   void cacheSize              (int, int);
   void maxLength              (int, int);

 protected:
   QString print_addr(in_addr_t);

 private:
  EQPacket* m_packet;
  QSpinBox* m_playbackSpeed;
  QLabel* m_packetTotal[MAXSTREAMS];
  QLabel* m_packetRecent[MAXSTREAMS];
  QLabel* m_packetAvg[MAXSTREAMS];
  QLabel* m_seqExp[MAXSTREAMS];
  QLabel* m_seqCur[MAXSTREAMS];
  QLabel* m_clientLabel;
  QLabel* m_sessionLabel;
  QLabel* m_serverPortLabel;
  QLabel* m_clientPortLabel;
  QLabel* m_cache[MAXSTREAMS];
  QLabel* m_maxLength[MAXSTREAMS];
  QLabel* m_filterLabel;

  int  m_packetStartTime[MAXSTREAMS];
  int  m_initialcount[MAXSTREAMS];
};

#endif // EQNETDIAG_H
