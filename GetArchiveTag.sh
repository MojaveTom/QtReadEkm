#!/bin/bash
#

#echo $0
#echo $PWD
git --git-dir=$PWD/.git log -1 --format='%H' > ArchiveTag.txt
