#!/bin/bash
##  Shell script to kill and restart ReadEKM program every day at 0705.

declare -i waitCount=20
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
####   MAKE SURE THERE IS A LINK TO THE CURRENT EXECUTIBLE
#### WHERE THIS SCRIPT EXPECTS IT TO BE.
$PWD/ReadEKM 300002570 &

# Reschedule this script to run at 0705 tomorrow.
at -fRefreshReadEKM.sh 0705 >/dev/null 2>&1
