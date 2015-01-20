#!/bin/bash

# @author Samuel Larkin
# @file lm_sort_filter.sh
# @brief Sort and filter an LM optimally for maximal compression.
# 
# @author Samuel Larkin
# 
# COMMENTS: Rewritten from Bruno Laferriere's version
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

print_nrc_copyright lm_sort_filter.sh 2006
export PORTAGE_INTERNAL_CALL=1

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: lm_sort_filter.sh [-h(elp)] [-v(erbose)] [-d(ebug)] [-lm]
       input output

  Takes an LM in ARPA format and outputs the ngram entries sorted regardless
  of their N.

  Define TMPDIR if you want to change where sort stores its temp files.

Options:
  -lm:          Rebuild a valid, sorted lm as output, in ARPA format.
                The output should have better compression ratio than the input.

  -h(elp):      print this help message
  -v(erbose):   increment the verbosity level by 1 (may be repeated)
  -d(ebug):     print debugging information

==EOF==

    exit 1
}

export LC_ALL=C

# Command line processing
DEBUG=
VERBOSE=0
REBUILD_VALID_LM=
while [ $# -gt 0 ]; do
   case "$1" in
   -lm)                 REBUILD_VALID_LM=1;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

# Reading positional arguments
test $# -eq 0   && error_exit "Missing input LM"
INPUT=$1; shift

if [ -n "$REBUILD_VALID_LM" ]; then
   test $# -eq 0   && error_exit "Missing output"
   OUTPUT=$1; shift
   if [[ $OUTPUT == "-" || $OUTPUT == "/dev/stdout" ]]; then
      error_exit "You must provide a regular file for output.";
   fi
fi

test $# -gt 0   && error_exit "Superfluous arguments $*"


# If the user was kind enough to provide a TMPDIR, use it.
if [ -n "$TMPDIR" ]; then
   SORT_DIR="TMPDIR=$TMPDIR"
fi


# Calculate the header's size
START_LINE=$(gzip -cdqf $INPUT | egrep -m1 -n '^\\1-gram' | cut -f1 -d':')


# ALWAYS work in a work directory
WORKDIR=`mktemp -d lm_sort_filter.$$.XXX` || error_exit "Cannot create temp workdir."
test -d $WORKDIR || mkdir -p $WORKDIR
verbose 1 "Created a tmp workdir: $WORKDIR"

# Split the input in parts based on Ngram size.
verbose 1 "Splitting input according to the ngram sections."
gzip -cqfd $INPUT \
| tail -n +$START_LINE \
| egrep -v '^\\end\\$' \
| egrep -v '^$' \
| perl -snle 'if(/^\\(\d)-grams:$/){close(FILE);open(FILE, ">'$WORKDIR'/part$1");} else{ print FILE $_;} END{close(FILE)}'

# Make sure splitting was done properly.
if [[ `ls $WORKDIR/* | \wc -l` == 0 ]]; then
   error_exit "Error splitting input lm.";
fi

# Make sure all parts are sorted
verbose 1 "Sorting all parts."
for p in $WORKDIR/part*; do
   if [ ! `sort -c -t'	' -k2,2 < $p 2> /dev/null` ]; then
      [[ $p =~ "part[0-9]+$" ]]
      warn "${BASH_REMATCH[0]} not sorted"
      echo "(LC_ALL=C $SORT_DIR sort -t'	' -k2,2 $p > $p.sorted) && mv $p.sorted $p"
   fi
done \
| run-parallel.sh - `ls $WORKDIR/ | \wc -l`


verbose 1 "Generating the output."
if [[ -n "$REBUILD_VALID_LM" ]]; then
   verbose 1 "Rebuilding a valid lm."
   if [[ $OUTPUT =~ ".gz$" ]]; then
      debug "Producing a compressed output"
      CAT="gzip"
   else
      debug "Producing uncompressed output"
      CAT="cat"
   fi

   # Create the final sorted lm.
   (
   echo
   echo "\\data\\"
   for p in $WORKDIR/part*; do
      [[ $p =~ "part([0-9]+)$" ]];
      N=${BASH_REMATCH[1]};
      test -n "$N" || error_exit "Unable to parse part number.";
      echo "ngram $N="`\wc -l < $p`;
   done
   echo
   for p in $WORKDIR/part*; do
      [[ $p =~ "part([0-9]+)$" ]];
      N=${BASH_REMATCH[1]};
      test -n "$N" || error_exit "Unable to parse part number.";
      echo "\\$N-grams:"
      cat $p;
      echo
   done
   echo "\\end\\"
   ) | $CAT > $OUTPUT

   TOTAL_ENTRIES=$(gzip -cqfd $INPUT | egrep -v '^$' | \wc -l)

   # Make sure all entries are still present
   verbose 1 "Checking output integrity."
   OUTPUT_ENTRIES=$(gzip -cqfd $OUTPUT | egrep -v '^$' | \wc -l)
   if (( $OUTPUT_ENTRIES != $TOTAL_ENTRIES )); then
      warn "Check your output, it's incomplete"
      warn "new: $OUTPUT_ENTRIES   old: $TOTAL_ENTRIES"
      exit 1;
   else
      rm -fr $WORKDIR
   fi
else
   # Tab-separated fields
   verbose 1 "Sorting all entries."
   LC_ALL=C $SORT_DIR sort -t'	' -k2,2 -m $WORKDIR/part* && rm -rf $WORKDIR
fi

