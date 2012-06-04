#!/bin/bash
# $Id$

# @file ictclas_run.sh
# @brief Add robustness to the ictclas pipeline.
# 
# @author Samuel Larkin based on Boxing Chen.
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

# Change the program name and year here
print_nrc_copyright prog.sh 2012

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: $0 [INPUT [OUTPUT]]

  Run ictclas_preprocessing.pl, ictclas & ictclas_postprocessing.pl.
  This script will completely process the input even though ictclas may have
  failed.

Options:

  -h(elp)     print this help message

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

INPUT="-"
OUTPUT="-"

test $# -eq 0   || INPUT=$1; shift
test $# -eq 0   || OUTPUT=$1; shift
test $# -gt 0   && error_exit "Superfluous arguments $*"

# [Update this block with your variables, or simply delete it, when you use
# this template]
if [ $DEBUG ]; then
   echo "
   INPUT=$INPUT
   OUTPUT=$OUTPUT
   REMAINING ARGS=$*
" >&2
fi

WORKDIR="ictclas.wd.$$"
if [[ "$INPUT" == "-" ]] || [[ "$INPUT" == "/dev/stdin" ]]; then
   mkdir -p $WORKDIR
   TMP_INPUT="$WORKDIR/in"
   zcat -f $INPUT > $TMP_INPUT
   INPUT=$TMP_INPUT
fi

if [[ "$OUTPUT" == "-" ]] || [[ "$OUTPUT" == "/dev/stdout" ]]; then
   mkdir -p $WORKDIR
   OUTPUT="$WORKDIR/out"
else
   warn "Overwriting $OUTPUT"
   cat /dev/null > $OUTPUT
fi
OUTPUTER=cat
[[ $OUTPUT =~ ".gz$" ]] && OUTPUTER=gzip

TOTAL_LINES=`wc -l < $INPUT`
PROCESSED_LINES=0
REMAINING_LINES=$(($TOTAL_LINES - $PROCESSED_LINES))

while [[ $REMAINING_LINES -gt 0 ]]; do
   zcat -f $INPUT |
   tail -$REMAINING_LINES |
   iconv -c -f UTF-8 -t CN-GB |
   ictclas_preprocessing.pl |
   ictclas |
   ictclas_postprocessing.pl |
   iconv -c -f CN-GB -t UTF-8 |
   $OUTPUTER >> $OUTPUT

   PROCESSED_LINES=`zcat -f $OUTPUT | \wc -l`
   REMAINING_LINES=$(($TOTAL_LINES - $PROCESSED_LINES))
done

[[ $OUTPUT =~ "$WORKDIR/out" ]] && cat $WORKDIR/out

[[ -d $WORKDIR ]] && rm -r $WORKDIR
