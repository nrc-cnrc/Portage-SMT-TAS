#!/bin/bash
# $Id$

# @file rescoremodel2canoearg.sh 
# @brief Convert a rescoring model in the format produced by rescoreloop.sh to
# an equivalant canoe argument string.
#
# @author George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

segFF=0
distFF=1
numTM=1
numLM=1

## 
## Usage: rescoremodel2canoearg.sh [-s S][-d D][-t T][-l L] rescoremodel
## 
## Convert a rescoring model in the format produced by rescoreloop.sh to an
## equivalant canoe argument string (written to stdout).
## Options:
## -d  Assume D distortion models (0 or 1) [1]
## -s  Assume S segmentation models (0 or 1) [0]
## -t  Assume T phrase models (1 or more) [1]
## -l  Assume L language models (1 or more) [1]
##

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh

[[ $PORTAGE_INTERNAL_CALL ]] ||
print_nrc_copyright rescoremodel2canoearg.sh 2006
export PORTAGE_INTERNAL_CALL=1

while getopts "l:d:s:t:h" flag; do
   case $flag in 
      h ) cat $0 | egrep '^##' | cut -c4-; exit 1;;
      d ) distFF=$OPTARG;;
      s ) segFF=$OPTARG;;
      t ) numTM=$OPTARG;;
      l ) numLM=$OPTARG;;
      * ) exit 1;;
   esac
done

shift $(($OPTIND - 1))

if [ $# != 1 ]; then
   cat $0 | egrep '^##' | cut -c4-
   exit 1
fi

model=$1
if [ ! -r $model ]; then
    error_exit "Cannot read model file $model."
fi

totlines=$((1 + $distFF + $segFF + $numLM + $numTM))
if [ `egrep -v '^[ ]*$' $model | wc -l` != $totlines ]; then
   error_exit "$model does not contain $totlines lines - check -d, -s, -l and -t values"
fi


d=0
if [ $distFF = 1 ]; then
    d=`egrep -v '^[ ]*$' $model | head -1 | cut -d' ' -f2`
fi
w=`egrep -v '^[ ]*$' $model | head -$((1+$distFF)) | tail -1 | cut -d' ' -f2`
sm=0
if [ $segFF = 1 ]; then
    sm=`egrep -v '^[ ]*$' $model | head -$((1+$distFF+$segFF)) | tail -1 | cut -d' ' -f2`
fi
lm=`egrep -v '^[ ]*$' $model | head -$(($numLM+$segFF+$distFF+1)) | tail -$numLM | cut -d' ' -f2 | paste -s -d':'`
tm=`egrep -v '^[ ]*$' $model | head -$(($numTM+$numLM+$segFF+$distFF+1)) | tail -$numTM | cut -d' ' -f2 | paste -s -d':'`

echo "-d $d -w $w -sm $sm -lm $lm -tm $tm"
