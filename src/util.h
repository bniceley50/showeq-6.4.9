/*
 *  util.h
 *  Copyright 2001-2005, 2019 by the respective ShowEQ Developers
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

#ifndef EQUTIL_H
#define EQUTIL_H

#ifdef __FreeBSD__
#include <sys/types.h>
long int lrint(double x);
long int lrintf(float x);
long int lroundf(float x);
#else
#include <cstdint>
#endif
#include <cstdio>
#include <QString>

#include "everquest.h"
#include "main.h"

// This define causes the crc table used by EQPacketForamt::calcCRC32 to be
// generated at runtime
//#define RUNTIME_CRCTAB 

#ifdef RUNTIME_CRCTAB
// this causes the generated crctab to be output to /tmp/crctab.h
//#define GENERATE_CRCTAB_H
#endif

char *print_addr (unsigned long addr);

QString Commanate (uint32_t number);

QString classString(uint8_t classVal);
QString spell_name (uint16_t spellId);
QString language_name (uint8_t langId);
QString skill_name (uint16_t skillId);
QString size_name (uint8_t size);

QString print_classes (uint16_t);
QString print_races (uint16_t);
QString print_slot (uint32_t slots);
QString print_material (uint8_t material);
QString print_skill (uint8_t skill);
QString print_faction (int32_t faction);
QString slot_to_name(int16_t slotnr);

uint32_t calc_exp (int level, uint16_t race, uint8_t class_);

int  mTime(void);
int calcMaxMana(int INT, int WIS, int daclass, int level);

QString reformatMoney (unsigned int uiCopper);

void fprintData(FILE* fp,
		uint32_t len,
		const uint8_t* data
		);

void diagFileWriteFail(QString filename);
void diagFileReadFail(QString filename);

uint32_t calcCRC32(const uint8_t* p, uint32_t length);
uint16_t calcCRC16(uint8_t* p, uint32_t length, uint32_t seed);

QStringList enumerateNetworkDevices();

// Templatized function to generate a string representing the bit pattern 
// of the passed in value.  
// Note: For use with integral data types.
template <class T>
QString bitstring(T value)
{
  QString bitstring;

  // Start with the high-bit and work backwards
  for (int i = (sizeof(T) * 8) - 1 ; i >= 0; i--)
    {
      // every 8 bits insert a space
      //      if (((i + 1) % 8) == 0)
      //	bitstring += " ";
      
      // if the value is a 1, insert a 1, otherwise insert a 0
      if (value & (1 << i))
	bitstring += "1";
      else
	bitstring += "0";

      if ((i != 0) && ((i % 8) == 0))
	bitstring += " ";
	
    }

  return bitstring;
}

#endif // EQUTIL_U
