# README.md
# QtReadEkm
\author Thomas A. DeMay
\date   2015

\par    Copyright (C) 2015  Thomas A. DeMay

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


A copy of the license can be found in LICENSE.txt in this project and source directory.

This is a Qt Project on Mac OS X Yosemite to read an EKM v.4 meter connected via EKM's "EKM Blink - RS-485 to USB Converter".
The EKM Omnimeter Pulse UL v.4 - Pulse has two Basic kWh Meter 14mm 120 volt meters connected to pulse 1 and pulse 2 inputs and
a water meter connected to pulse 3 input.  One output is used to power a EKM 485Bee and the other controls a relay that 
switches the rain sensor on a sprinkler timer.  The two 120V kWh meters are metering the two phases of the power to a well
water system.

The program reads the v.4 meter "A" and "B" data periodically and stores the responses basically un-intrepeted into a MySql 
database.  Since the program can be run indefinitely in the background, special files in the user's home directory are used
to signal events to the program.  These events are handled when the program would be idle.  Because there seems to be a memory
leak that I can't find, there is an included shell script to terminate the program and restart it daily.

The Notes.txt file has example SQL for pulling interesting (to me) information out of the database tables.
