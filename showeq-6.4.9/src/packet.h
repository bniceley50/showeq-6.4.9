/*
 *  packet.h
 *  Copyright 2000-2024 by the respective ShowEQ Developers
 *  Portions Copyright 2001-2003 Zaphod (dohpaz@users.sourceforge.net).
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

#ifndef _PACKET_H_
#define _PACKET_H_

#include <QObject>
#include <QTimer>
#include "packetcommon.h"
#include "packetinfo.h"

#if defined (__GLIBC__) && (__GLIBC__ < 2)
#error "Need glibc 2.1.3 or better"
#endif

#if (defined(__FreeBSD__) || defined(__linux__)) && defined(__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ < 2)
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;
#endif

#include <netinet/in.h>

//----------------------------------------------------------------------
// enumerated types
enum EQStreamPairs
{
  SP_World = 0x01,
  SP_Zone = 0x02
};

//----------------------------------------------------------------------
// forward declarations
class VPacket;
class PacketCaptureProviderThread;
class EQPacketStream;
class EQUDPIPPacketFormat;
class EQPacketTypeDB;
class EQPacketOPCodeDB;
class EQPacketOPCode;

//----------------------------------------------------------------------
// EQPacket
class EQPacket : public QObject
{
   Q_OBJECT 
 public:
   
   EQPacket(const QString& worldopcodesxml,
	    const QString& zoneopcodesxml,
	    uint16_t m_arqSeqGiveUp, 
	    QString m_device,
	    QString m_ip,
	    QString m_mac_address,
	    bool m_realtime,
        int snaplen,
        int buffersize,
	    bool m_session_tracking,
	    bool m_recordPackets,
	    int m_playbackPackets,
	    int8_t m_playbackSpeed, 
	    QObject *parent,
            const char *name);
   ~EQPacket();           
   void start(int delay = 0);
   void stop(void);

   const QString pcapFilter();
   int packetCount(int);
   const QString& ip();
   const QString& mac();
   const QString& device();
   in_addr_t clientAddr(void);
   in_port_t clientPort(void);
   in_port_t serverPort(void);
   uint8_t session_tracking_enabled(void);
   int playbackPackets(void);
   int playbackSpeed(void);
   size_t currentCacheSize(int);
   uint32_t currentMaxLength(int);
   uint16_t serverSeqExp(int);
   uint16_t arqSeqGiveUp(void);
   bool session_tracking(void);
   bool realtime(void);
   bool connect2(const QString& opcodeName, EQStreamPairs sp,
		 uint8_t dir, const char* payload,  EQSizeCheckType szt, 
		 const QObject* receiver, const char* member);
   int snaplen(void) { return m_snaplen; }
   int buffersize(void) { return m_buffersize; }
   void setSnapLen(int len) { m_snaplen = len; }
   void setBufferSize(int size) { m_buffersize = size; }

 public slots:
   void processPackets(void);
   void processPlaybackPackets(void);
   void incPlayback(void);
   void decPlayback(void);
   void setPlayback(int);
   void monitorIPClient(const QString& address);   
   void monitorMACClient(const QString& address);   
   void monitorNextClient();   
   void monitorDevice(const QString& dev);   
   void session_tracking(bool enable);
   void setArqSeqGiveUp(uint16_t giveUp);
   void setRealtime(bool val);
   void dispatchSessionKey(uint32_t sessionId, EQStreamID streamid,
      uint32_t sessionKey);

 protected slots:
   void closeStream(uint32_t sessionId, EQStreamID streamId);
   void unlatchClientPort();
   void lockOnClient(in_port_t serverPort, in_port_t clientPort, in_addr_t clientAddr);

 signals:
   // used for net_stats display
   void cacheSize(int, int);
   void seqReceive(int, int);
   void seqExpect(int, int);
   void numPacket(int, int);
   void maxLength(int, int);
   void resetPacket(int, int);
   void playbackSpeedChanged(int);
   void clientChanged(in_addr_t);
   void clientPortLatched(in_port_t);
   void serverPortLatched(in_port_t);
   void sessionTrackingChanged(uint8_t);
   void toggle_session_tracking(bool);
   void filterChanged(void);
   void stsMessage(const QString &, int = 0);

   // new logging
   void newPacket(const EQUDPIPPacketFormat& packet);
   void rawWorldPacket(const uint8_t* data, size_t len, uint8_t dir, 
		       uint16_t opcode);
   void decodedWorldPacket(const uint8_t* data, size_t len, uint8_t dir,
			   uint16_t opcode, const EQPacketOPCode* opcodeEntry);
   void decodedWorldPacket(const uint8_t* data, size_t len, uint8_t dir,
			   uint16_t opcode, const EQPacketOPCode* opcodeEntry,
               bool unknown);
   void rawZonePacket(const uint8_t* data, size_t len, uint8_t dir, 
		      uint16_t opcode);
   void decodedZonePacket(const uint8_t* data, size_t len, uint8_t dir,
			  uint16_t opcode, const EQPacketOPCode* opcodeEntry);
   void decodedZonePacket(const uint8_t* data, size_t len, uint8_t dir,
			  uint16_t opcode, const EQPacketOPCode* opcodeEntry,
			  bool unknown);

 private:
   void validateIP();

   PacketCaptureProviderThread* m_packetCapture;
   VPacket* m_vPacket;
   QTimer* m_timer;

   in_port_t m_serverPort;
   in_port_t m_clientPort;
   bool m_busy_decoding;
   bool m_detectingClient;
   in_addr_t m_client_addr;

   uint16_t m_arqSeqGiveUp;
   QString m_device;
   QString m_ip;
   QString m_mac;
   bool m_realtime;
   int m_snaplen;
   int m_buffersize;
   bool m_session_tracking;
   bool m_recordPackets;
   int m_playbackPackets;
   int8_t m_playbackSpeed; // Should be signed since -1 is pause

   EQPacketStream* m_client2WorldStream;
   EQPacketStream* m_world2ClientStream;
   EQPacketStream* m_client2ZoneStream;
   EQPacketStream* m_zone2ClientStream;
   EQPacketStream* m_streams[MAXSTREAMS];

   EQPacketTypeDB* m_packetTypeDB;
   EQPacketOPCodeDB* m_worldOPCodeDB;
   EQPacketOPCodeDB* m_zoneOPCodeDB;

   void connectStream(EQPacketStream* stream);
   void dispatchPacket   (int size, unsigned char *buffer);
   void dispatchPacket(EQUDPIPPacketFormat& packet);
 protected slots:
   void resetEQPacket();
   void dispatchWorldChatData (size_t len, uint8_t* data, uint8_t direction = 0);
};

inline in_addr_t EQPacket::clientAddr(void)
{
   return m_client_addr;
}

inline in_port_t EQPacket::clientPort(void)
{
  return m_clientPort;
}

inline in_port_t EQPacket::serverPort(void)
{
  return m_serverPort;
}

inline uint16_t EQPacket::arqSeqGiveUp(void)
{
  return m_arqSeqGiveUp;
}

inline bool EQPacket::session_tracking(void)
{
  return m_session_tracking;
}

inline int EQPacket::playbackPackets(void)
{
  return m_playbackPackets;
}

inline bool EQPacket::realtime(void)
{
  return m_realtime;
}

inline const QString& EQPacket::ip()
{
  return m_ip;
}

inline const QString& EQPacket::mac()
{
  return m_mac;
}

inline const QString& EQPacket::device()
{
  return m_device;
}

#endif // _PACKET_H_
