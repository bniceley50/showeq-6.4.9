/*
 *  sortitem.cpp
 *  Copyright 2001-2003, 2019 by the respective ShowEQ Developers
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

#include <cstdlib>
#include <cstdio>

#include <QTextStream>

#include "util.h"

#pragma message("Once our minimum supported Qt version is greater than 5.14, this check can be removed and ENDL replaced with Qt::endl")
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
#define ENDL Qt::endl
#else
#define ENDL endl
#endif

// This code used to be used to sort the old format item databases
// It is now a test program for use with regression testing the functions
// that provide printable names
int main (int, char *[])
{
  // open the data stream
  QTextStream out(stdout, QIODevice::WriteOnly);

  // format that the text will be output to.
  QString format = "%1:%2:%3\n";

  int hp = 6; // hexidecimal precision
  int dp = 8; // decimal precision
  int i, j;
  
  out << "Skills:" << ENDL;
  for (i = 0; i <=74; ++i)
    out << format.arg(i, dp).arg(i, hp, 16).arg(skill_name(i));
  out << "\n\n";

  out << "Language Names:\n";
  for (i = 0; i <= 24; ++i)
    out << format.arg(i, dp).arg(i, hp, 16).arg(language_name(i));
  out << "\n\n";

  out << "Race Strings:\n";
  out << format.arg(i, dp).arg(i, hp, 16).arg(print_races(0));
  // current top race (VAH) is 8192, aka 2^13 aka 1 << 12
  // making ALL = 0x3FFF
  i = 0x3fff;
  out << format.arg(i, dp).arg(i, hp, 16).arg(print_races(i));
  // include minimal test for Unknown valeus
  for (i = 1; i <= 16384; i = i << 1)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_races(i));
  // include minimal test for Unknown valeus
  for (i = 0, j = (1 << i); i <= 14; ++i, j += (1 << i))
    out << format.arg(j, dp).arg(j, hp, 16).arg(print_races(j));
  // don't test for Unknown values
  for (i = 13, j = (1 << i); i >= 0; --i , j += (1 << i))
    out << format.arg(j, dp).arg(j, hp, 16).arg(print_races(j));
  out << "\n\n";

  out << "Class Strings:\n";
  out << format.arg(i, dp).arg(i, hp, 16).arg(print_classes(0));
  // current top class (BST) is 16384, aka 2^14 aka 1 << 13
  // making ALL = 0x7fff
  i = 0x7fff;
  out << format.arg(i, dp).arg(i, hp, 16).arg(print_classes(i));
  // include minimal test for Unknown valeus
  for (i = 1; i <= 32768; i = i << 1)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_classes(i));
  // include minimal test for Unknown valeus
  for (i = 0, j = (1 << i); i <= 15; ++i, j += (1 << i))
    out << format.arg(j, dp).arg(j, hp, 16).arg(print_classes(j));
  // don't test for Unknown values
  for (i = 14, j = (1 << i); i >= 0; --i , j += (1 << i))
    out << format.arg(j, dp).arg(j, hp, 16).arg(print_classes(j));
  out << "\n\n";

  out << "Material Strings:\n";
  for (i = 0; i <= 0x18; ++i)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_material(i));
  out << "\n\n";

  out << "Skill Strings:\n";
  for (i = 0; i <= 5; ++i)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_skill(i));
  out << "\n\n";

  out << "Slot Strings:\n";
  out << format.arg(i, dp).arg(i, hp, 16).arg(print_slot(0));
  for (i = 1; i <= 0x00100000; i = i << 1)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_slot(i));
  for (i = 0, j = (1 << i); i <= 20; ++i, j += (1 << i))
    out << format.arg(j, dp).arg(j, hp, 16).arg(print_slot(j));
  for (i = 20, j = (1 << i); i >= 0; --i , j += (1 << i))
    out << format.arg(j, dp).arg(j, hp, 16).arg(print_slot(j));
  out << "\n\n";

  out << "Size Strings:\n";
  // include unknown size
  for (i = 0; i <= 5; ++i)
    out << format.arg(i, dp).arg(i, hp, 16).arg(size_name(i));
  out << "\n\n";

  out << "Faction Strings:\n";
  // include unknown sizes at either end (0 and 10)
  for (i = 0; i <= 10; ++i)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_faction(i));
  out << "\n\n";

  out << "Money Strings:\n";
  // Just use some representative examples (This isn't exhaustive by any means)
  i = 0;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 10;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 11;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 100;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 101;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 111;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1000;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1001;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1010;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1011;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1100;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1101;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1111;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 11110;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 11111;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 111100;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 1111000;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 11110000;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  i = 11111111;
  out << format.arg(i, dp).arg(i, hp, 16).arg(reformatMoney(i));
  out << "\n\n";

  out << "Item Skill Strings:\n";
  // test 1 higher then highest known
  for (i = 0; i <=28; i++)
    out << format.arg(i, dp).arg(i, hp, 16).arg(print_skill(i));
  out << "\n\n";
  out << "=================== DONE ===================\n";
}
