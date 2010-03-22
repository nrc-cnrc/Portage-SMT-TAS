#!/bin/bash
# $Id$

# @file tmtext_sort.sh 
# @brief sort a text TM by src or tgt phrases, then other.
# 
# @author Eric Joanis
# 
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/tpt directory
   BIN="`dirname $BIN`/utils"
fi
source $BIN/sh_utils.sh

print_nrc_copyright tmtext_sort.sh 2006
export PORTAGE_INTERNAL_CALL=1

usage() {
    for msg in "$@"; do
        echo $msg >&2
    done
    cat <<==EOF== >&2

Usage: tmtext_sort.sh [-h(elp)] [-1st] tmtext-file > sorted-tmtext-file

  Sort tmtext-file by the 2nd field (1st if -1st is given) then 1st (2nd).

==EOF==
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
    -h|-help)   usage;;
    -1st)       USE_FIRST=TRUE;;
    *)          break;;
    esac
    shift
done

TMTEXT_IN=$1;
if [ -z "$TMTEXT_IN" ]; then
    TMTEXT_IN=-
fi

if [ $TMTEXT_IN != "-" -a ! -r $TMTEXT_IN ]; then
    error_exit "Can't read file $TMTEXT_IN";
fi

# The sort command is only correct in C locale.
export LC_ALL=C

if [ "$USE_FIRST" == TRUE ]; then
    KEY="-k 1,1 -k 2,2"
else
    KEY="-k 2,2 -k 1,1"
fi

exec
    gzip -cdfq $TMTEXT_IN |
    perl -pe 's/ \|\|\| /\t/g' |
    sort -t'	' $KEY |
    sed 's/	/ ||| /g'

# Benchmarking notes: The above code was benchmarked with perl and sed.  Perl
# does the first substitution about 10% faster than sed, but sed does the
# second one 50% faster than Perl, hence the choices retained above.

# Rejected and benchmarking code:
#/usr/bin/time sed 's/ ||| /	/g' $TMTEXT_IN |
#    /usr/bin/time sort -t'	' -k 2,2 -k 1,1 |
#    /usr/bin/time sed 's/	/ ||| /g'
#/usr/bin/time perl -pe 's/ \|\|\| /\t/g' $TMTEXT_IN |
#    /usr/bin/time sort -t'	' -k 2,2 -k 1,1 |
#    /usr/bin/time perl -pe 's/\t/ ||| /g'
