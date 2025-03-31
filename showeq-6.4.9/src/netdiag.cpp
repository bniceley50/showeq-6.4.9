/*
 *  netdiag.cpp
 *  Copyright 2002-2007, 2019 by the respective ShowEQ Developers
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

#include <QPushButton>
#include <QShortcut>
#include <QGridLayout>
#include <QLabel>

#include "main.h"
#include "netdiag.h"
#include "packet.h"
#include "util.h"

NetDiag::NetDiag(EQPacket* packet, QWidget* parent, const char* name = NULL)
  : SEQWindow("NetDiag", "ShowEQ - Network Diagnostics", parent, name),
    m_packet(packet),
    m_playbackSpeed(NULL)
{

  QWidget* mainWidget = new QWidget();
  setWidget(mainWidget);

  //  setResizeEnabled(false);
  // get preferences
  QGridLayout* tmpGrid = new QGridLayout(mainWidget);
  tmpGrid->addItem(new QSpacerItem(5, 0), 0, 3);
  tmpGrid->addItem(new QSpacerItem(5, 0), 0, 6);
  tmpGrid->addItem(new QSpacerItem(0, 5), 1, 0);
  tmpGrid->addItem(new QSpacerItem(0, 5), 3, 0);
  tmpGrid->addItem(new QSpacerItem(0, 5), 6, 0);
  tmpGrid->addItem(new QSpacerItem(0, 5), 8, 0);

  int row = 0;
  int col = 0;

  // create labels to display client & server info
  tmpGrid->addWidget(new QLabel("Network ", this), row, col++);
  tmpGrid->addWidget(new QLabel("Client: ", this), row, col++);
  m_clientLabel = new QLabel(this);
  tmpGrid->addWidget(m_clientLabel, row, col++);
  col++;
  tmpGrid->addWidget(new QLabel("ServerPort: ", this), row, col++);
  m_serverPortLabel = new QLabel(this);
  tmpGrid->addWidget(m_serverPortLabel, row, col++);
  col++;
  tmpGrid->addWidget(new QLabel("ClientPort: ", this), row, col++);
  m_clientPortLabel = new QLabel(this);
  tmpGrid->addWidget(m_clientPortLabel, row, col++);

  // second row of network info
  row++; col = 1;
  tmpGrid->addWidget(new QLabel("Device: ", this), row, col++);
  QLabel* tmpLabel = new QLabel(this);
  tmpLabel->setText(m_packet->device());
  tmpGrid->addWidget(tmpLabel, row, col++);
  col++;
  tmpGrid->addWidget(new QLabel("MAC: ", this), row, col++);
  tmpLabel = new QLabel(this);
  tmpLabel->setText(m_packet->mac());
  tmpGrid->addWidget(tmpLabel, row, col++);
  col++;
  tmpGrid->addWidget(new QLabel("Track: ", this), row, col++);
  m_sessionLabel = new QLabel(this);
  tmpGrid->addWidget(m_sessionLabel, row, col++);
  m_sessionLabel->setNum(m_packet->session_tracking_enabled());

  clientChanged(m_packet->clientAddr());
  serverPortLatched(m_packet->serverPort());
  clientPortLatched(m_packet->clientPort());

  // 3rd row of network info
  row++; col = 0;
  tmpGrid->addWidget(new QLabel("Pcap Thread: ", this), row, col++);
  tmpGrid->addWidget(new QLabel("Realtime: ", this), row, col++);
  tmpLabel = new QLabel(this);
  tmpLabel->setText(QString::number(m_packet->realtime()));
  tmpGrid->addWidget(tmpLabel, row, col++);
  row++; col = 1;
  tmpGrid->addWidget(new QLabel("Filter: ", this), row, col++);
  m_filterLabel = new QLabel(this);
  m_filterLabel->setText(m_packet->pcapFilter());
  tmpGrid->addWidget(m_filterLabel, row, col, 1, 5);

  // stream specific statistics
  row++; row++; col = 0;

  QString eqStreams[] = {"client->world", "world->client", "client->zone", "zone->client"};

  for (int a = 0; a < MAXSTREAMS; a++)
  {
     tmpGrid->addWidget(new QLabel(eqStreams[a], this), row, col++);
     tmpGrid->addWidget(new QLabel("Max Length:", this), row, col++);
     m_maxLength[a] = new QLabel("unknown", this);
     m_maxLength[a]->setNum((int)m_packet->currentMaxLength(a));
     tmpGrid->addWidget(m_maxLength[a], row, col++);
     col++;

     row++; col = 0;

     // packet throughput
     tmpGrid->addWidget(new QLabel("Packets ", this), row, col++);
     tmpGrid->addWidget(new QLabel("Total: ", this), row, col++);
     m_packetTotal[a] = new QLabel("count", this);
     tmpGrid->addWidget(m_packetTotal[a], row, col++);
     col++;
     tmpGrid->addWidget(new QLabel("Recent: ", this), row, col++);
     m_packetRecent[a] = new QLabel("recent", this);
     tmpGrid->addWidget(m_packetRecent[a], row, col++);
     col++;
     tmpGrid->addWidget(new QLabel("Rate: ", this), row, col++);
     m_packetAvg[a] = new QLabel("avg", this);
     tmpGrid->addWidget(m_packetAvg[a], row, col++);
     resetPacket(m_packet->packetCount(a), a);

     row++; col = 0;

     // network status
     tmpGrid->addWidget(new QLabel("Status ", this), row, col++);
     tmpGrid->addWidget(new QLabel("Cached: ", this), row, col++);
     m_cache[a] = new QLabel("cached", this);
     m_cache[a]->setNum((int)m_packet->currentCacheSize(a));
     tmpGrid->addWidget(m_cache[a], row, col++);
     col++;
     tmpGrid->addWidget(new QLabel("SeqExp: ", this), row, col++);
     m_seqExp[a] = new QLabel("seqexp", this);
     tmpGrid->addWidget(m_seqExp[a], row, col++);
     col++;
     tmpGrid->addWidget(new QLabel("SeqCur: ", this), row, col++);
     m_seqCur[a] = new QLabel("seqcur", this);
     tmpGrid->addWidget(m_seqCur[a], row, col++);
     row++; row++; col = 0;
     seqExpect(m_packet->serverSeqExp(a), a);
     m_seqCur[a]->setText("????");
  }

  if (m_packet->playbackPackets())
  {
    tmpGrid->addWidget(new QLabel("Playback ", this), row, col++);
    tmpGrid->addWidget(new QLabel("Rate: ", this), row, col++);
    m_playbackSpeed = new QSpinBox(this);
    m_playbackSpeed->setObjectName("speed");
    m_playbackSpeed->setMinimum(-1);
    m_playbackSpeed->setMaximum(9);
    m_playbackSpeed->setSingleStep(1);
    m_playbackSpeed->setSuffix("x");
    m_playbackSpeed->setSpecialValueText("Pause");
    m_playbackSpeed->setWrapping(true);
    tmpGrid->addWidget(m_playbackSpeed, row, col++, Qt::AlignLeft);

    m_playbackSpeed->setValue(m_packet->playbackSpeed());

    QKeySequence key;

    key = pSEQPrefs->getPrefKey("IncPlaybackSpeedKey", preferenceName(), "Ctrl+X");
    QShortcut *incPlayback_shortcut = new QShortcut(key, this);
    connect (incPlayback_shortcut, SIGNAL(activated()), m_packet, SLOT(incPlayback()));

    key = pSEQPrefs->getPrefKey("IncPlaybackSpeedKey", preferenceName(), "Ctrl+Z");
    QShortcut *decPlayback_shortcut = new QShortcut(key, this);
    connect (decPlayback_shortcut, SIGNAL(activated()), m_packet, SLOT(decPlayback()));
  }

  // supply the LCD's with signals
  connect (m_packet, SIGNAL(cacheSize(int, int)), 
	   this, SLOT(cacheSize(int, int)));
  connect (m_packet, SIGNAL(seqExpect (int, int)), 
	   this, SLOT(seqExpect(int, int)));
  connect (m_packet, SIGNAL(seqReceive (int, int)), 
	   this, SLOT(seqReceive(int, int)));
  connect (m_packet, SIGNAL(clientChanged(in_addr_t)),
	   this, SLOT(clientChanged(in_addr_t)));
  connect (m_packet, SIGNAL(clientPortLatched(in_port_t)),
	   this, SLOT(clientPortLatched(in_port_t)));
  connect (m_packet, SIGNAL(serverPortLatched(in_port_t)),
	   this, SLOT(serverPortLatched(in_port_t)));
  connect (m_packet, SIGNAL(sessionTrackingChanged(uint8_t)),
	   this, SLOT(sessionTrackingChanged(uint8_t)));
  connect (m_packet, SIGNAL(numPacket(int, int)),
	   this, SLOT(numPacket(int, int)));
  connect (m_packet, SIGNAL(resetPacket(int, int)),
	   this, SLOT(resetPacket(int, int)));
  connect (m_packet, SIGNAL(filterChanged()),
	   this, SLOT(filterChanged()));
  connect (m_packet, SIGNAL(maxLength(int, int)),
	   this, SLOT(maxLength(int, int)));

  if (m_playbackSpeed)
  {
    connect (m_playbackSpeed, SIGNAL(valueChanged(int)),
	     m_packet, SLOT(setPlayback(int)));
    connect (m_packet, SIGNAL(playbackSpeedChanged(int)),
	     m_playbackSpeed, SLOT(setValue(int)));
  }
}

NetDiag::~NetDiag()
{
}

void NetDiag::seqReceive(int seq, int stream)
{
  QString disp;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  disp = QString::asprintf("%4.4x", seq);
#else
  disp.sprintf("%4.4x", seq);
#endif
  m_seqCur[stream]->setText(disp);
}

void NetDiag::seqExpect(int seq, int stream)
{
  QString disp;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  disp = QString::asprintf("%4.4x", seq);
#else
  disp.sprintf("%4.4x", seq);
#endif
  m_seqExp[stream]->setText(disp);
}

void NetDiag::clientChanged(in_addr_t addr)
{
  QString disp, tmp;
  disp = print_addr(addr);

  uint8_t sessionState = m_packet->session_tracking_enabled();
  m_sessionLabel->setNum(sessionState);

  switch (sessionState)
  {
  case 2:
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    tmp = QString::asprintf(":%d", 
		m_packet->clientPort());
#else
    tmp.sprintf(":%d", 
		m_packet->clientPort());
#endif
    disp += tmp;
    break;
  case 1:
    disp += QString(":?");
    break;
  default:
    break;
  }
  m_clientLabel->setText(disp);
}

void NetDiag::clientPortLatched(in_port_t clientPort)
{
  QString disp, tmp;
  uint32_t addr = m_packet->clientAddr();

  disp = print_addr(addr);
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  tmp = QString::asprintf(":%d", clientPort);
#else
  tmp.sprintf(":%d", clientPort);
#endif

  disp += tmp;

  m_clientLabel->setText(disp);
}

void NetDiag::serverPortLatched(in_port_t port)
{
  m_serverPortLabel->setText(QString::number(port));
}

void NetDiag::sessionTrackingChanged(uint8_t sessionTrackState)
{
  QString disp, tmp;
  disp = print_addr(m_packet->clientAddr());

  m_sessionLabel->setNum(sessionTrackState);
  switch(sessionTrackState)
  {
  case 1:
    m_clientLabel->setText(disp + ":?");
    break;
  case 2:
    break;
  default:
    m_clientLabel->setText(disp);
    break;
  }
}

void NetDiag::filterChanged()
{
  m_filterLabel->setText(m_packet->pcapFilter());
}

void NetDiag::resetPacket(int num, int stream)
{
  // if passed 0 reset the average
  m_packetStartTime[stream] = mTime();
  m_initialcount[stream] = num;

  m_packetTotal[stream]->setText(QString::number(num));
  m_packetRecent[stream]->setText("0");
  m_packetAvg[stream]->setText("0.0");
}

void NetDiag::numPacket(int num, int stream)
{
  // start the timer of not started
  if (!m_packetStartTime)
    m_packetStartTime[stream] = mTime();

   QString tempStr;

   m_packetTotal[stream]->setText(QString::number(num));

  // update five times per sec
  static int lastupdate = 0;
  if ( (mTime() - lastupdate) < 1000)
    return;
  lastupdate = mTime();

   int numdelta = num - m_initialcount[stream];
   m_packetRecent[stream]->setText(QString::number(num));
   int delta = mTime() - m_packetStartTime[stream];
   if (numdelta && delta)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     tempStr = QString::asprintf("%2.1f", 
		     (float) (numdelta<<10) / (float) delta);
#else
     tempStr.sprintf("%2.1f", 
		     (float) (numdelta<<10) / (float) delta);
#endif
   else   
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     tempStr = QString::asprintf("0.0");
#else
     tempStr.sprintf("0.0");
#endif

   m_packetAvg[stream]->setText(tempStr);
}

void NetDiag::cacheSize(int size, int stream)
{
  m_cache[stream]->setNum(size);
}

void NetDiag::maxLength(int len, int streamId)
{
  m_maxLength[streamId]->setNum(len);
}

QString NetDiag::print_addr(in_addr_t  addr)
{
#ifdef DEBUG_PACKET
   qDebug ("print_addr()");
#endif /* DEBUG_PACKET */
  QString paddr;

#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
  paddr = QString::asprintf ( "%d.%d.%d.%d",
		  addr & 0x000000ff,
		  (addr & 0x0000ff00) >> 8,
		  (addr & 0x00ff0000) >> 16,
		  (addr & 0xff000000) >> 24
                  );
#else
  paddr.sprintf ( "%d.%d.%d.%d",
		  addr & 0x000000ff,
		  (addr & 0x0000ff00) >> 8,
		  (addr & 0x00ff0000) >> 16,
		  (addr & 0xff000000) >> 24
                  );
#endif

   return paddr;
}

#ifndef QMAKEBUILD
#include "netdiag.moc"
#endif

