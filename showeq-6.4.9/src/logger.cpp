/*
 *  logger.cpp - packet/data logging class
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

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <QString>
#include <QList>

#include "logger.h"

SEQLogger::SEQLogger(FILE *fp, QObject* parent, const char* name)
  : QObject(parent)
{
    setObjectName(name);
    m_fp = fp;
    m_errOpen = false;
}

SEQLogger::SEQLogger(const QString& fname, QObject* parent, const char* name)
  : QObject(parent)
{
    setObjectName(name);
    m_fp = NULL;
    m_filename = fname;
    m_errOpen = false;
}

bool SEQLogger::open()
{
  if (m_fp)
    return true;

  m_fp = fopen(m_filename.toLatin1().data(),"a");

  if (!m_fp)
  {
    if (!m_errOpen)
    {
      ::fprintf(stderr, "Error opening %s: %s (will keep trying)\n",
              m_filename.toLatin1().data(), strerror(errno));
      m_errOpen = true;
    }

    return false;
  }

  m_errOpen = false;

  if (!m_file.open(m_fp, QIODevice::Append | QIODevice::WriteOnly))
    return false;

  m_out.setDevice(&m_file);

  return true;
}

void SEQLogger::flush()
{
  m_file.flush();
}


int SEQLogger::outputf(const char *fmt, ...)
{
  va_list args;
  int count;

  if (!m_fp)
    return 0;

  va_start(args, fmt);
  count = vfprintf(m_fp, fmt, args);
  va_end(args);

  return count;
}

int SEQLogger::output(const void* data, int length)
{
  int i;
  int count = 0;
  unsigned char *ptr = (unsigned char *) data;
  
  for(i = 0; i < length; i++,ptr++)
    count += printf("%.2X", *ptr);
  
  return count;
}

// prints up the passed in data to the file pointer
void SEQLogger::outputData(uint32_t len,
			  const uint8_t* data)
{
  if (!m_fp)
    return;

  char hex[128];
  char asc[128];
  char tmp[32];
  
  hex[0] = 0;
  asc[0] = 0;
  unsigned int c;
  
  for (c = 0; c < len; c ++)
  {
    if ((!(c % 16)) && c)
    {
      fprintf (m_fp, "%03d | %s | %s \n", c - 16, hex, asc);
      hex[0] = 0;
      asc[0] = 0;
    }
    
    sprintf (tmp, "%02x ", data[c]);
    strcat (hex, tmp);
    
    if ((data[c] >= 32) && (data[c] <= 126))
      sprintf (tmp, "%c", data[c]);
    
    else
      strcpy (tmp, ".");
    
    strcat (asc, tmp);
    
  }
  
  if (c % 16)
    c = c - (c % 16);
  
  else
    c -= 16;
  
  fprintf (m_fp, "%03d | %-48s | %s \n\n", c, hex, asc);
}

#ifndef QMAKEBUILD
#include "logger.moc"
#endif

