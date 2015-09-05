#!/bin/bash
#
#
#    GetArchiveTag.sh -- Shell script to kill and restart ReadEKM program every day at 0705.
#    Copyright (C) 2015  Thomas A. DeMay
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 3 of the License, or
#    any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# *
# */
#
#  This script creates a special file in the user's home directory that the
# program looks for.  When found, the program quits gracefully.
#
#    Script is run in the source directory.
#

declare -i waitCount=20     #  Will cause us to give up clean quit after 200 seconds.
# Get pid of ReadEKM process -- look for "ReadEKM" surrounded by word breaks.
#         Not just "ReadEKM" because that could be in some other process.
# wpid is NOT empty if the ReadEKM application is running, and the status is true.
if wpid=$(ps -AcU"tom" | grep "\<ReadEKM\>")
then
    touch $HOME/.CloseReadEKM       # Create .CloseReadEKM file if not exist.
    while wpid=$(ps -AcU"tom" | grep "\<ReadEKM\>")
    do      ##  Wait for ReadEKM program to see .CloseReadEKM file and quit.
        sleep 10
        ## But don't wait forever.
        if [ $((--waitCount)) -lt 0 ]; then break; fi
    done
fi

# if wpid is not empty, clean termination above did not work.
if [ -n "$wpid" ]; then
    wpid=$(echo $wpid | cut -d" " -f1)  # Extract the pid
    kill $wpid                          # kill the process
    rm $HOME/.CloseReadEKM    # remove the .CloseReadEKM file
                                        # so it won't kill the process
                                        # immediately after starting.
    # sleep for 10 seconds
    sleep 10
fi

#  Sleep till 1 sec after the next minute.
sleep $(( 61 - ( $(date -j +%s) % 60 ) ))

# restart ReadEKM program
####   MAKE SURE THERE IS A LINK TO THE CURRENT EXECUTABLE
#### WHERE THIS SCRIPT EXPECTS IT TO BE.
$PWD/ReadEKM 300002570 &

# Reschedule this script to run at 0705 tomorrow.
at -fRefreshReadEKM.sh 0705 >/dev/null 2>&1
