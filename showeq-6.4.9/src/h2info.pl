#!/usr/bin/perl

#  Copyright 2003-2005, 2019 by the respective ShowEQ Developers
#
#  This file is part of ShowEQ.
#  http://www.sourceforge.net/projects/seq
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# print "Argument Count: ", $#ARGV, "\n";
die "Insufficient arguments!\n" if ($#ARGV < 1);
$source = $ARGV[0];
$dest = $ARGV[1];

print "Generating '$dest' from '$source'\n";
open(SOURCE_FILE, "$source") || die "Can't find $source $OPCODES_H: $!\n";
open(DEST_FILE, ">$dest") || die "Can't create $dest: $DEST_FILE: $!\n";

print DEST_FILE "/*
 * $dest
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sf.net/
 *
 *  Copyright 2001-2003 Zaphod (dohpaz\@users.sourceforge.net). 
 *
 * This file is automatically generated by packeth2structs.pl from the 
 * forward declaration of structures is $source
 * 
 * **** Do Not Hand Edit *********
 *
 */\n";

while ($line = <SOURCE_FILE>) {
    if ($line =~ m/^struct\s+(\S*)\s+/)
    {
	print DEST_FILE "AddStruct($1);\n";
    }
}

close(SOURCE_FILE);
close(DEST_FILE)
