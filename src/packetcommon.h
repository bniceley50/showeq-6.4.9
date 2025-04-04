/*
 *  packetcommon.h
 *  Copyright 2000-2005, 2019 by the respective ShowEQ Developers
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

#ifndef _PACKETCOMMON_H
#define _PACKETCOMMON_H

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#if !defined(__BYTE_ORDER)
#if defined(BYTE_ORDER)
#define __BYTE_ORDER	BYTE_ORDER
#define __LITTLE_ENDIAN	LITTLE_ENDIAN
#define __BIG_ENDIAN	BIG_ENDIAN
#else
#error "BYTE_ORDER should be LITTLE_ENDIAN or BIG_ENDIAN (the latter is untested)"
#endif
#endif
#else
#include <cstdint>
#endif

#ifdef __linux__
#include <endian.h>
#endif

#include <netdb.h>

//----------------------------------------------------------------------
// Constants
const char* const AUTOMATIC_CLIENT_IP = "127.0.0.0";

const in_port_t WorldServerGeneralMinPort = 9000;
const in_port_t WorldServerGeneralMaxPort = 9015;
const in_port_t WorldServerChatPort = 9876;
const in_port_t WorldServerChat2Port = 9875; // xgame tells, mail
const in_port_t LoginServerMinPort = 15900;
const in_port_t LoginServerMaxPort = 15910;
const in_port_t ChatServerPort = 5998;

// Preference constants for VPacket.Playback.
#define PLAYBACK_OFF 0
#define PLAYBACK_FORMAT_SEQ 1
#define PLAYBACK_FORMAT_TCPDUMP 2

//----------------------------------------------------------------------
// Enumerated types
enum EQStreamID 
{
  unknown_stream = -1,
  client2world = 0, 
  world2client = 1,
  client2zone = 2, 
  zone2client = 3,
  MAXSTREAMS = 4,
};

// direction the data is coming from
enum EQDir
{
  DIR_Client = 0x01,
  DIR_Server = 0x02,
};


//----------------------------------------------------------------------
// Useful inline functions
#if (defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN))
inline uint16_t eqntohuint16(const uint8_t* data)
{
  return (uint16_t)((data[0] << 8) | data[1]);
}

inline int16_t eqntohint16(const uint8_t* data)
{
  return (int16_t)((data[0] << 8) | data[1]);
}

inline  uint32_t eqntohuint32(const uint8_t* data)
{
  return (uint32_t)((data[0] << 24) | (data[1] << 16) |
		    (data[2] << 8) | data[3]);
}

inline int32_t eqntohint32(const uint8_t* data)
{
  return (int32_t)((data[0] << 24) | (data[1] << 16) |
		   (data[2] << 8) | data[3]);
}

inline uint16_t eqtohuint16(const uint8_t* data)
{
  return *(uint16_t*)data;
}

inline int16_t eqtohint16(const uint8_t* data)
{
  return *(int16_t*)data;
}

inline uint32_t eqtohuint32(const uint8_t* data)
{
  return *(uint32_t*)data;
}

inline int32_t eqtohint32(const uint8_t* data)
{
  return *(int32_t*)data;
}
#else
#warning "BigEndian hasn't been tested."
inline uint16_t eqntohuint16(const uint8_t* data)
{
  return *(uint16_t*)data;
}

inline int16_t eqntohint16(const uint8_t* data)
{
  return *(int16_t*)data;
}

inline uint32_t eqntohuint32(const uint8_t* data)
{
  return *(uint32_t*)data;
}

inline int32_t eqntohint32(const uint8_t* data)
{
  return *(int32_t*)data;
}

inline uint16_t eqtohuint16(const uint8_t* data)
{
  return (uint16_t)((data[0] << 8) | data[1]);
}

inline int16_t eqtohint16(const uint8_t* data)
{
  return (int16_t)((data[0] << 8) | data[1]);
}

inline  uint32_t eqtohuint32(const uint8_t* data)
{
  return (uint32_t)((data[0] << 24) | (data[1] << 16) |
		    (data[2] << 8) | data[3]);
}

inline int32_t eqtohint32(const uint8_t* data)
{
  return (int32_t)((data[0] << 24) | (data[1] << 16) |
		   (data[2] << 8) | data[3]);
}
#endif

#endif // _PACKETCOMMON_H

