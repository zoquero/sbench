#!/bin/bash

#
# Simple script to test sequential write access to filesystem
#
# * Creates a temporary file 
# * Fills it with zeros and flushes the file
# * Returns the number of
#     MegaBytes/second (not MebiBytes/second) needed for filling + 2nd flushing
#
# @author zoquero@gmail.com
# @since  20161026
# @version 1.0 : Initial
#

function usage() {
  echo "Creates a temporary file sequentially"
  echo ""
  echo "Usage: $0 folder blockNum blockSize (-v)"
  echo "	folder    : where to create the temporary file"
  echo "	blockNum  : number of blocks"
  echo "	blockSize : block size"
  echo "	-v        : increase verbosity (optional, must be last param)"
  echo ""
  echo "Ex: $0 /tmp 20480 1024"
  exit 1
}

##########
# Params #
##########
export LC_ALL=C  # set decimal dot
verbose="" # true if not blank
folder=$1
template=$(basename $0).tmpfile.XXXXXXXX
blockNum=$2
blockSize=$3

if [ "$4" == "-v" ]; then
  verbose="y"
fi

[[ "$verbose" ]] && echo "folder=$folder template=$template blockNum=$blockNum blockSize=$blockSize"

if [ -z "$folder" -o -z "$blockNum" -o -z "$blockSize" ]; then
  echo "Missing parameters"
  usage
fi
if [ ! -d "$folder" ]; then
  echo "The folder '$folder' doesn't exist"
  usage
fi

re='^[0-9]+$'
if ! [[ $blockNum =~ $re ]] ; then
  echo "blockNum must be a number"
  usage
fi
if ! [[ $blockSize =~ $re ]] ; then
  echo "blockSize must be a number"
  usage
fi

###################
# Create tmp file #
###################
tmpfile=$(mktemp -p "$folder" -q $template)
if [ ! -f "$tmpfile" ]; then
  echo "Couldn't create temporary file on '$folder'. Exiting."
  exit 2
fi

[[ "$verbose" ]] && echo "tmpfile=$tmpfile"

#####################
# Fill the tmp file #
#####################
# https://romanrm.net/dd-benchmark
[[ "$verbose" ]] && echo "dd if=/dev/zero of=\"$tmpfile\" bs=$blockSize count=$blockNum conv=fsync"
timeInSeconds=$(/usr/bin/time -f "%e" sh -c "dd if=/dev/zero of=\"$tmpfile\" bs=$blockSize count=$blockNum conv=fsync >/dev/null 2>&1" 2>&1)
if [ $? -ne 0 ]; then
  echo "Couldn't fill the file"
  exit 3
fi
[[ "$verbose" ]] && echo "timeInSeconds=[$timeInSeconds]"

##########################
# Calc, clean and return #
##########################
sizeInBytes=$(echo "$blockSize*$blockNum" | bc -l)
[[ "$verbose" ]] && echo "sizeInBytes=[$sizeInBytes]"
megaBytesPerSecRaw=$(echo "$sizeInBytes/$timeInSeconds/1000000" | bc -l)
megaBytesPerSec=$(printf "%.2f\n" $megaBytesPerSecRaw)

[[ "$verbose" ]] && echo "megaBytesPerSecRaw=[$megaBytesPerSecRaw]"
[[ "$verbose" ]] && echo "megaBytesPerSec   =[$megaBytesPerSec]"

rm "$tmpfile"
if [ -f "$tmpfile" ]; then
  echo "Couldn't remove '$tmpfile'."
  exit 2
fi

[[ "$verbose" ]] && echo "timeInSeconds=[$timeInSeconds] , sizeInBytes=[$sizeInBytes] , sequential write throghput=[$megaBytesPerSec] * 10E6 B/s"
echo "$megaBytesPerSec"
exit 0

