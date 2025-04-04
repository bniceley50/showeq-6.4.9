/*
 *  packet.cpp
 *  Copyright 2000-2024 by the respective ShowEQ Developers
 *  Portions Copyright 2001-2004,2007 Zaphod (dohpaz@users.sourceforge.net).
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

/* Implementation of Packet class */
#include <cstdio>
#include <unistd.h>
#include <netdb.h>

#ifdef __FreeBSD__
#include "packet.h"
#endif
#include <netinet/if_ether.h>

#include <QTimer>
#include <QFileInfo>

#include "everquest.h"
#include "packet.h"
#include "packetcommon.h"
#include "packetcapture.h"
#include "packetformat.h"
#include "packetstream.h"
#include "packetinfo.h"
#include "vpacket.h"
#include "everquest.h"
#include "diagnosticmessages.h"

//----------------------------------------------------------------------
// Macros

//#define DEBUG_PACKET
//#undef DEBUG_PACKET

// The following defines are used to diagnose packet handling behavior
// this define is used to diagnose packet processing (in dispatchPacket mostly)
//#define PACKET_PROCESS_DIAG 3 
// verbosity level 0-n

// this define is used to diagnose cache handling (in dispatchPacket mostly)
//#define PACKET_CACHE_DIAG 3 
// verbosity level (0-n)

// diagnose opcode DB issues
//#define  PACKET_OPCODEDB_DIAG 1

// diagnose structure size changes
#define PACKET_PAYLOAD_SIZE_DIAG 1

// Packet version is a unique number that should be bumped every time packet
// structure (ie. encryption) changes.  It is checked by the VPacket feature
// (currently the date of the last packet structure change)
#define PACKETVERSION  40101

//----------------------------------------------------------------------
// constants

//----------------------------------------------------------------------
// Here begins the code


//----------------------------------------------------------------------
// EQPacket class methods

/* EQPacket Class - Sets up packet capturing */

////////////////////////////////////////////////////
// Constructor
EQPacket::EQPacket(const QString& worldopcodesxml,
		   const QString& zoneopcodesxml,
		   uint16_t arqSeqGiveUp, 
		   QString device,
		   QString ip,
		   QString mac_address,
		   bool realtime,
           int snaplen,
           int buffersize,
		   bool sessionTrackingFlag,
		   bool recordPackets,
		   int playbackPackets,
		   int8_t playbackSpeed, 
		   QObject * parent, const char *name)
  : QObject (parent),
    m_packetCapture(NULL),
    m_vPacket(NULL),
    m_timer(NULL),
    m_busy_decoding(false),
    m_arqSeqGiveUp(arqSeqGiveUp),
    m_device(device),
    m_ip(ip),
    m_mac(mac_address),
    m_realtime(realtime),
    m_snaplen(snaplen),
    m_buffersize(buffersize),
    m_session_tracking(sessionTrackingFlag),
    m_recordPackets(recordPackets),
    m_playbackPackets(playbackPackets),
    m_playbackSpeed(playbackSpeed)
{
  setObjectName(name);
  // create the packet type db
  m_packetTypeDB = new EQPacketTypeDB();

#ifdef PACKET_OPCODEDB_DIAG
  m_packetTypeDB->list();
#endif

  // create the world opcode db
  m_worldOPCodeDB = new EQPacketOPCodeDB();

  // load the world opcode db
  if (!m_worldOPCodeDB->load(*m_packetTypeDB, worldopcodesxml))
    seqFatal("Error loading '%s'!", worldopcodesxml.toLatin1().data());

#ifdef PACKET_OPCODEDB_DIAG
  m_worldOPCodeDB->list();
#endif

  //m_worldOPCodeDB->save("/tmp/worldopcodes.xml");

  // create the zone opcode db
  m_zoneOPCodeDB = new EQPacketOPCodeDB();

  // load the zone opcode db
  if (!m_zoneOPCodeDB->load(*m_packetTypeDB, zoneopcodesxml))
    seqFatal("Error loading '%s'!", zoneopcodesxml.toLatin1().data());

#ifdef PACKET_OPCODEDB_DIAG
  m_zoneOPCodeDB->list();
#endif

  //m_zoneOPCodeDB->save("/tmp/zoneopcodes.xml");
  
  // Setup the data streams

  // Setup client -> world stream
  m_client2WorldStream = new EQPacketStream(client2world, DIR_Client, 
					    m_arqSeqGiveUp, *m_worldOPCodeDB,
					    this, "client2world");
  connectStream(m_client2WorldStream);

  // Setup world -> client stream
  m_world2ClientStream = new EQPacketStream(world2client, DIR_Server,
					    m_arqSeqGiveUp, *m_worldOPCodeDB,
					    this, "world2client");
  connectStream(m_world2ClientStream);

  // Setup client -> zone stream
  m_client2ZoneStream = new EQPacketStream(client2zone, DIR_Client,
					  m_arqSeqGiveUp, *m_zoneOPCodeDB,
					  this, "client2zone");
  connectStream(m_client2ZoneStream);

  // Setup zone -> client stream
  m_zone2ClientStream = new EQPacketStream(zone2client, DIR_Server,
					   m_arqSeqGiveUp, *m_zoneOPCodeDB,
					   this, "zone2client");
  connectStream(m_zone2ClientStream);

  // Initialize convenient streams array
  m_streams[client2world] = m_client2WorldStream;
  m_streams[world2client] = m_world2ClientStream;
  m_streams[client2zone] = m_client2ZoneStream;
  m_streams[zone2client] = m_zone2ClientStream;

  // no client/server ports yet
  m_clientPort = 0;
  m_serverPort = 0;

  if (m_ip.isEmpty() && m_mac.isEmpty())
  {
    seqInfo("No address specified. Defaulting to client auto-detect");
    m_ip = AUTOMATIC_CLIENT_IP;
  }

  validateIP();

  if (m_playbackPackets == PLAYBACK_OFF)
  {
    // create the pcap object and initialize, either with MAC or IP
    m_packetCapture = new PacketCaptureThread(m_snaplen, m_buffersize);
    if (m_mac.length() == 17)
    {
      seqInfo("Listening for client MAC: %s", m_mac.toLatin1().data());

      m_packetCapture->start(m_device.toLatin1().data(),
              m_mac.toLatin1().data(),
              m_realtime, MAC_ADDRESS_TYPE );
    }
    else
    {
      if (m_detectingClient)
        seqInfo("Listening for next client seen. (you must zone for this to work!)");
      else
        seqInfo("Listening for client: %s", m_ip.toLatin1().data());

      m_packetCapture->start(m_device.toLatin1().data(),
              m_ip.toLatin1().data(),
              m_realtime, IP_ADDRESS_TYPE );
    }
    emit filterChanged();
  }
  else if (m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
  {
    // Create the pcap object and initialize with the file input given
    m_packetCapture = new PacketCaptureThread(m_snaplen, m_buffersize);

    QString filename = pSEQPrefs->getPrefString("Filename", "VPacket");

    m_packetCapture->startOffline(filename.toLatin1().data(), m_playbackSpeed);
    seqInfo("Playing back packets from '%s' at speed '%d'",
      filename.toLatin1().data(), m_playbackSpeed);
  }

  // Flag session tracking properly on streams
  session_tracking(sessionTrackingFlag);

  // if running setuid root, then give up root access, since the PacketCapture
  // is the only thing that needed it.
  if ((geteuid() == 0) && (getuid() != geteuid()))
    setuid(getuid()); 

  /* Create timer object */
  m_timer = new QTimer (this);
  
  if (m_playbackPackets == PLAYBACK_OFF || 
          m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
  {
    // Normal pcap packet handler
    connect (m_timer, SIGNAL (timeout ()), this, SLOT (processPackets ()));
  }
  else
  {
    // Special internal playback handler
    connect (m_timer, SIGNAL (timeout ()), this, SLOT (processPlaybackPackets ()));
  }
  
  /* setup VPacket */
  m_vPacket = NULL;
  
  QString section = "VPacket";
  // First param to VPacket is the filename
  // Second param is playback speed:  0 = fast as poss, 1 = 1X, 2 = 2X etc
  if (pSEQPrefs->isPreference("Filename", section))
  {
    QString filename = pSEQPrefs->getPrefString("Filename", section);

    if (m_recordPackets)
    {
      m_vPacket = new VPacket(filename.toLatin1().data(), 1, true);
      // Must appear befire next call to getPrefString, which uses a static string
      seqInfo("Recording packets to '%s' for future playback", filename.toLatin1().data());

      if (!pSEQPrefs->getPrefString("FlushPackets", section).isNull())
          m_vPacket->setFlushPacket(true);
    }
    else if (m_playbackPackets == PLAYBACK_FORMAT_SEQ)
    {
      m_vPacket = new VPacket(filename.toLatin1().data(), 1, false);
      m_vPacket->setCompressTime(pSEQPrefs->getPrefInt("CompressTime", section, 0));
      m_vPacket->setPlaybackSpeed(m_playbackSpeed);

      seqInfo("Playing back packets from '%s' at speed '%d'", filename.toLatin1().data(),

              m_playbackSpeed);
    }
  }
  else
  {
    m_recordPackets = 0;
    m_playbackPackets = PLAYBACK_OFF;
  }
}

//helper function to verify specified IP is a valid IP or hostname, and/or
//to set up auto detection.
void EQPacket::validateIP()
{
  struct in_addr  ia;
  struct hostent *he;

  if (m_ip.isEmpty() || m_ip == AUTOMATIC_CLIENT_IP)
  {
    /* Substitute "special" IP which is interpreted
       to set up a different filter for picking up new sessions */
    inet_aton (AUTOMATIC_CLIENT_IP, &ia);
  }
  else if (inet_aton (m_ip.toLatin1().data(), &ia) == 0)
  {
    he = gethostbyname(m_ip.toLatin1().data());
    if (he)
    {
        memcpy (&ia, he->h_addr_list[0], he->h_length);
    }
    else
    {
        // If the IP or host is invalid, default to auto-detect, rather
        // than immediately exiting (the previous behavior)
        seqWarn("Invalid address or hostname: %s", m_ip.toLatin1().data());
        seqWarn("Defaulting to client auto-detect");
        m_ip = AUTOMATIC_CLIENT_IP;
        inet_aton (AUTOMATIC_CLIENT_IP, &ia);
    }
  }
  m_client_addr = ia.s_addr;
  m_ip = inet_ntoa(ia);

  m_detectingClient = (m_ip == AUTOMATIC_CLIENT_IP);

}

////////////////////////////////////////////////////
// Destructor
EQPacket::~EQPacket()
{

  if (m_packetCapture != NULL)
  {
    // stop any packet capture 
    m_packetCapture->stop();

    // delete the object
    delete m_packetCapture;
  }

  // try to close down VPacket cleanly
  if (m_vPacket != NULL)
  {
    // make sure any data is flushed to the file
    m_vPacket->Flush();

    // delete VPacket
    delete m_vPacket;
  }

  if (m_timer != NULL)
  {
    // make sure the timer is stopped
    m_timer->stop();

    // delete the timer
    delete m_timer;
  }

  resetEQPacket();

  delete m_client2WorldStream;
  delete m_world2ClientStream;
  delete m_client2ZoneStream;
  delete m_zone2ClientStream;

  if (m_packetTypeDB)
  {
    delete m_packetTypeDB;
  }
  if (m_zoneOPCodeDB)
  {
    delete m_zoneOPCodeDB;
  }
  if (m_worldOPCodeDB)
  {
    delete m_worldOPCodeDB;
  }
}

/* Start the timer to process packets */
void EQPacket::start (int delay)
{
#ifdef DEBUG_PACKET
   qDebug ("start()");
#endif /* DEBUG_PACKET */
   m_timer->start (delay);
}

/* Stop the timer to process packets */
void EQPacket::stop (void)
{
#ifdef DEBUG_PACKET
   qDebug ("stop()");
#endif /* DEBUG_PACKET */
   m_timer->stop ();
}

/* Reads packets and processes waiting packets */
void EQPacket::processPackets (void)
{
  /* Make sure we are not called while already busy */
  if (m_busy_decoding)
     return;

  /* Set flag that we are busy decoding */
  m_busy_decoding = true;
  
  unsigned char buffer[BUFSIZ]; 
  short size;
  
  /* fetch them from pcap */
  while ((size = m_packetCapture->getPacket(buffer)))
  {
    /* Now.. we know the rest is an IP udp packet concerning the
     * host in question, because pcap takes care of that.
     */
      
    /* Now we assume its an everquest packet */
    if (m_recordPackets)
    {
      time_t now = time(NULL);
      m_vPacket->Record((const char *) buffer, size, now, PACKETVERSION);
    }
      
    dispatchPacket (size - sizeof (struct ether_header),
		  (unsigned char *) buffer + sizeof (struct ether_header) );
  }

  /* Clear decoding flag */
  m_busy_decoding = false;
}

////////////////////////////////////////////////////
// Reads packets and processes waiting packets from playback file
void EQPacket::processPlaybackPackets (void)
{
#ifdef DEBUG_PACKET
//   qDebug ("processPackets()");
#endif /* DEBUG_PACKET */
  /* Make sure we are not called while already busy */
  if (m_busy_decoding)
    return;

  /* Set flag that we are busy decoding */
  m_busy_decoding = true;

  unsigned char  buffer[8192];
  int            size;

  /* in packet playback mode fetch packets from VPacket class */
  time_t now;
  int timein = mTime();
  int i = 0;
    
  long version = PACKETVERSION;
  
  // decode packets from the playback buffer
  do
  {
    size = m_vPacket->Playback((char *) buffer, sizeof(buffer), &now, &version);
    
    if (size)
    {
      i++;
	
      if (PACKETVERSION == version)
      {
	dispatchPacket ( size - sizeof (struct ether_header),
		       (unsigned char *) buffer + sizeof (struct ether_header)
		       );
      }
      else
      {
	seqWarn("Error:  The version of the packet stream has " \
		 "changed since '%s' was recorded - disabling playback",
		 m_vPacket->getFileName());

	// stop the timer, nothing more can be done...
	stop();

	break;
      }
    }
    else
      break;
  } while ( (mTime() - timein) < 100);

  // check if we've reached the end of the recording
  if (m_vPacket->endOfData())
  {
    seqInfo("End of playback file '%s' reached."
	    "Playback Finished!",
	    m_vPacket->getFileName());

    // stop the timer, nothing more can be done...
    stop();
  }

  /* Clear decoding flag */
  m_busy_decoding = false;
}

/////////////////////////////////////////////////////////
// Connect the given stream's signals to the proper slots
void EQPacket::connectStream(EQPacketStream* stream)
{
  // Packet logging
  switch (stream->streamID())
  {
    case zone2client:
    case client2zone:
    {
      // Zone server stream
      connect(stream,
        SIGNAL(rawPacket(const uint8_t*, size_t, uint8_t, uint16_t)),
        this,
        SIGNAL(rawZonePacket(const uint8_t*, size_t, uint8_t, uint16_t)));

      connect(stream,
        SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
        this,
        SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));

      connect(stream,
        SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
        this,
        SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
    }
    break;
    case world2client:
    case client2world:
    {
      // World server stream
      connect(stream,
        SIGNAL(rawPacket(const uint8_t*, size_t, uint8_t, uint16_t)),
        this,
        SIGNAL(rawWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t)));

      connect(stream,
        SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
        this,
        SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));

      connect(stream,
        SIGNAL(decodedPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
        this,
        SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
    }
    break;
    default :
    {
      return;
    }
  }

  // Debugging
  connect(stream,
      SIGNAL(cacheSize(int, int)),
      this,
      SIGNAL(cacheSize(int, int)));
  connect(stream,
      SIGNAL(seqReceive(int, int)),
      this,
      SIGNAL(seqReceive(int, int)));
  connect(stream,
      SIGNAL(seqExpect(int, int)),
      this,
      SIGNAL(seqExpect(int, int)));
  connect(stream,
      SIGNAL(numPacket(int, int)),
      this,
      SIGNAL(numPacket(int, int)));

  // Session handling
  connect(stream,
      SIGNAL(sessionTrackingChanged(uint8_t)),
      this,
      SIGNAL(sessionTrackingChanged(uint8_t)));
  connect(stream,
      SIGNAL(lockOnClient(in_port_t, in_port_t, in_addr_t)),
      this,
      SLOT(lockOnClient(in_port_t, in_port_t, in_addr_t)));
  connect(stream,
      SIGNAL(closing(uint32_t, EQStreamID)),
      this,
      SLOT(closeStream(uint32_t, EQStreamID)));
  connect(stream,
      SIGNAL(sessionKey(uint32_t, EQStreamID, uint32_t)),
      this,
      SLOT(dispatchSessionKey(uint32_t, EQStreamID, uint32_t)));
  connect(stream,
      SIGNAL(maxLength(int, int)),
      this,
      SIGNAL(maxLength(int, int)));
}

////////////////////////////////////////////////////
// This function decides the fate of the Everquest packet 
// and dispatches it to the correct packet stream for handling function
void EQPacket::dispatchPacket(int size, unsigned char *buffer)
{
#ifdef DEBUG_PACKET
  qDebug ("EQPacket::dispatchPacket()");
#endif /* DEBUG_PACKET */

  // Create an object to parse the packet
  EQUDPIPPacketFormat packet(buffer, size, false);

  dispatchPacket(packet);

  // signal a new packet. This has to be at the end so that the session is
  // filled in if possible, so that it can report on crc errors properly
  emit newPacket(packet);
}

void EQPacket::dispatchPacket(EQUDPIPPacketFormat& packet)
{
  // Detect client by world server port traffic...
  if (m_detectingClient &&
        (packet.getSourcePort() >= WorldServerGeneralMinPort
         && packet.getSourcePort() <= WorldServerGeneralMaxPort))
  {
    m_ip = packet.getIPv4DestA();
    m_client_addr = packet.getIPv4DestN();
    m_detectingClient = false;
    emit clientChanged(m_client_addr);
    seqInfo("Client Detected: %s", m_ip.toLatin1().data());
  }
  else if (m_detectingClient &&
            (packet.getDestPort() >= WorldServerGeneralMinPort
             && packet.getDestPort() <= WorldServerGeneralMaxPort))
  {
    m_ip = packet.getIPv4SourceA();
    m_client_addr = packet.getIPv4SourceN();
    m_detectingClient = false;
    emit clientChanged(m_client_addr);
    seqInfo("Client Detected: %s", m_ip.toLatin1().data());
  }

  // Dispatch based on known streams
  if ((packet.getDestPort() == ChatServerPort) ||
      (packet.getSourcePort() == ChatServerPort))
  {
    // Drop chat server traffic
    return;
  }
  else if ((packet.getDestPort() == WorldServerChatPort) ||
      (packet.getSourcePort() == WorldServerChatPort))
  {
    // Drop cross-server chat traffic
    return;
  }
  else if ((packet.getDestPort() == WorldServerChat2Port) ||
      (packet.getSourcePort() == WorldServerChat2Port))
  {
    // Drop email and cross-game chat traffic
    return;
  }
  else if (((packet.getDestPort() >= LoginServerMinPort) &&
      (packet.getDestPort() <= LoginServerMaxPort)) ||
      ((packet.getSourcePort() >= LoginServerMinPort) &&
      (packet.getSourcePort() <= LoginServerMaxPort)))
  {
    // Drop login server traffic
    return;
  }
  else if ((packet.getDestPort() >= WorldServerGeneralMinPort &&
            packet.getDestPort() <= WorldServerGeneralMaxPort) ||
           (packet.getSourcePort() >= WorldServerGeneralMinPort &&
            packet.getSourcePort() <= WorldServerGeneralMaxPort))
  {
    // World server traffic. Dispatch it.
    if (packet.getIPv4SourceN() == m_client_addr)
    {
      m_client2WorldStream->handlePacket(packet);
    }
    else
    {
      m_world2ClientStream->handlePacket(packet);
    }
  }
  else
  {
    // Anything else we assume is zone server traffic.
    if (packet.getIPv4SourceN() == m_client_addr)
    {
      m_client2ZoneStream->handlePacket(packet);
    }
    else
    {
      m_zone2ClientStream->handlePacket(packet);
    }
  }
} /* end dispatchPacket() */

////////////////////////////////////////////////////
// Handle zone2client stream closing
void EQPacket::closeStream(uint32_t sessionId, EQStreamID streamId)
{
  // If this is the zone server session closing, reset the pcap filter to
  // a non-exclusive form
  if ((streamId == zone2client || streamId == client2zone) &&
         (m_playbackPackets == PLAYBACK_OFF || 
          m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP))
  {
    m_packetCapture->setFilter(m_device.toLatin1().data(), m_ip.toLatin1().data(),
            m_realtime, IP_ADDRESS_TYPE, 0, 0);
    emit filterChanged();
  }

  // Pass the close onto the streams
  m_client2WorldStream->close(sessionId, streamId, m_session_tracking);
  m_world2ClientStream->close(sessionId, streamId, m_session_tracking);
  m_client2ZoneStream->close(sessionId, streamId, m_session_tracking);
  m_zone2ClientStream->close(sessionId, streamId, m_session_tracking);

  // If we just closed the zone server session, unlatch the client port
  if (streamId == zone2client || streamId == client2zone)
  {
    unlatchClientPort();

    seqInfo("EQPacket: SessionDisconnect detected, awaiting next zone session,  pcap filter: EQ Client %s",
            (m_ip == AUTOMATIC_CLIENT_IP) ? "auto-detect" : m_ip.toLatin1().data());
  }
}

// Unlatch a locked-on client port, so the next client is detected correctly
void EQPacket::unlatchClientPort()
{
    m_clientPort = 0;
    m_serverPort = 0;
    emit clientPortLatched(m_clientPort);
}


////////////////////////////////////////////////////
// Locks onto a specific client port (for session tracking)
void EQPacket::lockOnClient(in_port_t serverPort, in_port_t clientPort, in_addr_t clientAddr)
{
  m_serverPort = serverPort;
  m_clientPort = clientPort;
  m_client_addr = clientAddr;

  in_addr ia = inet_makeaddr(ntohl(m_client_addr), ntohl(m_client_addr));

  if (m_playbackPackets == PLAYBACK_OFF ||
          m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
  {
    if (m_mac.length() == 17)
    {
      m_packetCapture->setFilter(m_device.toLatin1().data(),
              m_mac.toLatin1().data(),
              m_realtime,
              MAC_ADDRESS_TYPE, 0,
              m_clientPort);
      emit filterChanged();
    }
    else
    {
      m_packetCapture->setFilter(m_device.toLatin1().data(),
              m_ip.toLatin1().data(),
              m_realtime,
              IP_ADDRESS_TYPE, 0,
              m_clientPort);
      emit filterChanged();
    }
  }

  // Wanted this message even if we're running on playback...
  if (m_mac.length() == 17)
  {
    seqInfo("EQPacket: SessionRequest detected, pcap filter: EQ Client %s, Client port %d. Server port %d",
      m_mac.toLatin1().data(), m_clientPort, m_serverPort);
  }
  else
  {
    seqInfo("EQPacket: SessionRequest detected, pcap filter: EQ Client %s, Client port %d. Server port %d",
      inet_ntoa(ia), m_clientPort, m_serverPort);
  }

  emit clientPortLatched(m_clientPort);
}

void EQPacket::dispatchSessionKey(uint32_t sessionId, EQStreamID streamid,
  uint32_t sessionKey)
{
  m_client2WorldStream->receiveSessionKey(sessionId, streamid, sessionKey);
  m_world2ClientStream->receiveSessionKey(sessionId, streamid, sessionKey);
  m_client2ZoneStream->receiveSessionKey(sessionId, streamid, sessionKey);
  m_zone2ClientStream->receiveSessionKey(sessionId, streamid, sessionKey);
}

///////////////////////////////////////////
//EQPacket::dispatchWorldChatData  
// note this dispatch gets just the payload
void EQPacket::dispatchWorldChatData (size_t len, uint8_t *data, 
				      uint8_t dir)
{
#ifdef DEBUG_PACKET
  qDebug ("dispatchWorldChatData()");
#endif /* DEBUG_PACKET */
  if (len < 10)
    return;
  
  uint16_t opCode = eqntohuint16(data);

  switch (opCode)
  {
  default:
    seqDebug("%04x - %d (%s)", opCode, len,
	    ((dir == DIR_Server) ? 
	     "WorldChatServer --> Client" : "Client --> WorldChatServer"));
  }
}

///////////////////////////////////////////
// Returns the current playback speed
int EQPacket::playbackSpeed(void)
{
  if (m_vPacket)
    return m_vPacket->playbackSpeed();
  else
    return m_packetCapture->getPlaybackSpeed();
}

///////////////////////////////////////////
// Set the packet playback speed
void EQPacket::setPlayback(int speed)
{
  if (m_vPacket)
  {
    m_vPacket->setPlaybackSpeed(speed);
  }
  else
  {
    m_packetCapture->setPlaybackSpeed(speed);
  }
    
  QString string("");
    
  if (speed == 0)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
    string = QString::asprintf("Playback speed set Fast as possible");
#else
    string.sprintf("Playback speed set Fast as possible");
#endif
  else if (speed < 0)
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     string = QString::asprintf("Playback paused (']' to resume)");
#else
     string.sprintf("Playback paused (']' to resume)");
#endif
  else
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
     string = QString::asprintf("Playback speed set to %d", speed);
#else
     string.sprintf("Playback speed set to %d", speed);
#endif

  emit stsMessage(string, 5000);

  emit resetPacket(m_client2WorldStream->packetCount(), client2world);
  emit resetPacket(m_world2ClientStream->packetCount(), world2client);
  emit resetPacket(m_client2ZoneStream->packetCount(), client2zone);
  emit resetPacket(m_zone2ClientStream->packetCount(), zone2client);

  emit playbackSpeedChanged(speed);
}

///////////////////////////////////////////
// Increment the packet playback speed
void EQPacket::incPlayback(void)
{
  int x;

  if (m_vPacket)
  {
    x = m_vPacket->playbackSpeed();
  }
  else
  {
    x = m_packetCapture->getPlaybackSpeed();
  }
    
  switch(x)
  {
	// if we were paused go to 1X not full speed
    case -1:
      x = 1;
      break;
	
    // can't go faster than full speed
    case 0:
      return;
	
    case 9:
      x = 0;
      break;
	
    default:
      x += 1;
      break;
  }
    
  setPlayback(x);
}

///////////////////////////////////////////
// Decrement the packet playback speed
void EQPacket::decPlayback(void)
{
  int x;

  if (m_vPacket)
  {
    x = m_vPacket->playbackSpeed();
  }
  else
  {
    x = m_packetCapture->getPlaybackSpeed();
  }

  switch(x)
  {
    // paused
    case -1:
      return;
	  break;

    // slower than 1 is paused
    case 1:
      x = -1;
      break;
	
    // if we were full speed goto 9
    case 0:
      x = 9;
      break;
	
    default:
      x -= 1;
      break;
  }
    
  setPlayback(x);
}

///////////////////////////////////////////
// Set the IP address of the client to monitor
void EQPacket::monitorIPClient(const QString& ip)
{
  m_ip = ip;

  validateIP();

  emit clientChanged(m_client_addr);

  resetEQPacket();

  seqInfo("Listening for IP client: %s", (m_ip == AUTOMATIC_CLIENT_IP) ? "auto-detect" : m_ip.toLatin1().data());
  if (m_playbackPackets == PLAYBACK_OFF ||
          m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
  {
    m_packetCapture->setFilter(m_device.toLatin1().data(),
            m_ip.toLatin1().data(),
            m_realtime, 
            IP_ADDRESS_TYPE, 0, 0);
    emit filterChanged();
  }
}

///////////////////////////////////////////
// Set the MAC address of the client to monitor
void EQPacket::monitorMACClient(const QString& mac)
{
  m_mac = mac;
  struct in_addr  ia;
  inet_aton (AUTOMATIC_CLIENT_IP, &ia);
  m_detectingClient = true;
  m_client_addr = ia.s_addr;
  emit clientChanged(m_client_addr);

  resetEQPacket();

  if (!m_mac.isEmpty() && m_mac.length() == 17)
  {
    seqInfo("Listening for MAC client: %s", m_mac.toLatin1().data());

    if (m_playbackPackets == PLAYBACK_OFF ||
            m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
    {
        m_packetCapture->setFilter(m_device.toLatin1().data(),
                m_mac.toLatin1().data(),
                m_realtime,
                MAC_ADDRESS_TYPE, 0, 0);
        emit filterChanged();
    }
  }
  else
  {
      seqWarn("Invalid MAC specified.  Defaulting to auto-detect of next client.");
      monitorNextClient();
  }
}

///////////////////////////////////////////
// Monitor the next client seen
void EQPacket::monitorNextClient()
{
  m_detectingClient = true;
  m_ip = AUTOMATIC_CLIENT_IP;
  struct in_addr  ia;
  inet_aton (m_ip.toLatin1().data(), &ia);
  m_client_addr = ia.s_addr;
  emit clientChanged(m_client_addr);

  resetEQPacket();

  seqInfo("Listening for next client seen. (you must zone for this to work!)");

  if (m_playbackPackets == PLAYBACK_OFF ||
          m_playbackPackets == PLAYBACK_FORMAT_TCPDUMP)
  {
    m_packetCapture->setFilter(m_device.toLatin1().data(), NULL,
            m_realtime,
            DEFAULT_ADDRESS_TYPE, 0, 0);
    emit filterChanged();
  }
}

///////////////////////////////////////////
// Monitor for packets on the specified device
void EQPacket::monitorDevice(const QString& dev)
{
  // set the device to use
  m_device = dev;

  // make sure we aren't playing back packets
  if (m_playbackPackets != PLAYBACK_OFF)
    return;

  // stop the current packet capture
  m_packetCapture->stop();

  validateIP();

  resetEQPacket();

  // restart packet capture
  if (m_mac.length() == 17)
  {
    seqInfo("Listening for client MAC: %s", m_mac.toLatin1().data());

    m_packetCapture->start(m_device.toLatin1().data(),
            m_mac.toLatin1().data(),
            m_realtime, MAC_ADDRESS_TYPE );
  }
  else
  {
    if (m_detectingClient)
      seqInfo("Listening for next client seen. (you must zone for this to work!)");
    else
      seqInfo("Listening for client: %s", m_ip.toLatin1().data());

    m_packetCapture->start(m_device.toLatin1().data(),
            m_ip.toLatin1().data(),
            m_realtime, IP_ADDRESS_TYPE );
  }

  emit filterChanged();
}

///////////////////////////////////////////
// Set the session tracking state
void EQPacket::session_tracking(bool enable)
{
  m_session_tracking = enable;
  m_client2WorldStream->setSessionTracking(m_session_tracking);
  m_world2ClientStream->setSessionTracking(m_session_tracking);
  m_client2ZoneStream->setSessionTracking(m_session_tracking);
  m_zone2ClientStream->setSessionTracking(m_session_tracking);
  emit sessionTrackingChanged(m_session_tracking);

}

///////////////////////////////////////////
// Set the current ArqSeqGiveUp
void EQPacket::setArqSeqGiveUp(uint16_t giveUp)
{
  // a sanity check, if the user set it to below 32, they're prolly nuts
  if (giveUp >= 32)
    m_arqSeqGiveUp = giveUp;

  // a sanity check, if the user set it to below 32, they're prolly nuts
  if (m_arqSeqGiveUp >= 32)
    giveUp = m_arqSeqGiveUp;
  else
    giveUp = 32;

  m_client2WorldStream->setArqSeqGiveUp(giveUp);
  m_world2ClientStream->setArqSeqGiveUp(giveUp);
  m_client2ZoneStream->setArqSeqGiveUp(giveUp);
  m_zone2ClientStream->setArqSeqGiveUp(giveUp);
}

void EQPacket::setRealtime(bool val)
{
  m_realtime = val;
}

bool EQPacket::connect2(const QString& opcodeName, EQStreamPairs sp,
		 uint8_t dir, const char* payload,  EQSizeCheckType szt, 
		 const QObject* receiver, const char* member)
{
  bool res = false;

  if (sp & SP_World)
  {
    if (dir & DIR_Client)
      res = m_client2WorldStream->connect2(opcodeName, payload, szt, receiver, member);
    if (dir & DIR_Server)
      res = m_world2ClientStream->connect2(opcodeName, payload, szt, receiver, member);
  }
  if (sp & SP_Zone)
  {
    if (dir & DIR_Client)
      res = m_client2ZoneStream->connect2(opcodeName, payload, szt, receiver, member);
    if (dir & DIR_Server)
      res = m_zone2ClientStream->connect2(opcodeName, payload, szt, receiver, member);
  }

  return res;
}

///////////////////////////////////////////
// Reset EQPacket's state
void EQPacket::resetEQPacket()
{
  m_client2WorldStream->reset();
  m_client2WorldStream->setSessionTracking(m_session_tracking);
  m_world2ClientStream->reset();
  m_world2ClientStream->setSessionTracking(m_session_tracking);
  m_client2ZoneStream->reset();
  m_client2ZoneStream->setSessionTracking(m_session_tracking);
  m_zone2ClientStream->reset();
  m_zone2ClientStream->setSessionTracking(m_session_tracking);

  unlatchClientPort();
}

///////////////////////////////////////////
// Return the current pcap filter
const QString EQPacket::pcapFilter()
{
  // make sure we aren't playing back packets
  if (m_playbackPackets != PLAYBACK_OFF)
    return QString("Playback");

  return m_packetCapture->getFilter();
}


int EQPacket::packetCount(int stream)
{
  return m_streams[stream]->packetCount();
}

uint8_t EQPacket::session_tracking_enabled(void)
{
  return m_zone2ClientStream->sessionTracking();
}

size_t EQPacket::currentCacheSize(int stream)
{
  return m_streams[stream]->currentCacheSize();
}

uint32_t EQPacket::currentMaxLength(int streamId)
{
    return m_streams[streamId]->getMaxLength();
}

uint16_t EQPacket::serverSeqExp(int stream)
{
  return m_streams[stream]->arqSeqExp();
}

#ifndef QMAKEBUILD
#include "packet.moc"
#endif

