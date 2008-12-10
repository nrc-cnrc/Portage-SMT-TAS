#!/bin/bash
# $Id$

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

echo 'lm_sort_filter.sh, NRC-CNRC, Copyright (c) 2006 - 2008, Her Majesty in Right of Canada' >&2

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

# error_exit "some error message" "optionnally a second line of error message"
# will exit with an error status, print the specified error message(s) on
# STDERR.
error_exit() {
   echo -n `basename $0`: "" >&2
   for msg in "$@"; do
      echo $msg >&2
   done
   echo "Use -h for help." >&2
   exit 1
}

# Verify that enough args remain on the command line
# syntax: one_arg_check <args needed> $# <arg name>
# Note that this function expects to be in a while/case structure for
# handling parameters, so that $# still includes the option itself.
# exits with error message if the check fails.
arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

# Warning message
warn()
{
   echo "WARNING: $*" >&2
}

# Debug message
debug()
{
   test -n "$DEBUG" && echo "<D> $*" >&2
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
fi

test $# -gt 0   && error_exit "Superfluous arguments $*"


# If the user was kind enough to provide a TMPDIR, use it.
if [ -n "$TMPDIR" ]; then
   SORT_DIR="TMPDIR=$TMPDIR"
fi


# Calculate the header's size
START_LINE=$(gzip -cdqf $INPUT | egrep -n '^\\1-gram' | head -1 | cut -f1 -d':')


# ALWAYS work in a work directory
WORKDIR=lm_sort_filter.$$
test -d $WORKDIR || mkdir $WORKDIR

# Split the input in parts based on Ngram size.
gzip -cqfd $INPUT \
| tail +$START_LINE \
| egrep -v '^\\end\\$' \
| egrep -v '^$' \
| perl -snle 'if(/^\\(\d)-grams:$/){close(FILE);open(FILE, ">'$WORKDIR'/part$1");} else{ print FILE $_;} END{close(FILE)}'

# Make sure all parts are sorted
for p in $WORKDIR/part*; do
   if [ ! `sort -c -t'	' -k2,2 < $p 2> /dev/null` ]; then
      [[ $p =~ "./(part[0-9]+)$" ]]
      warn "${BASH_REMATCH[1]} not sorted"
      echo "(LC_ALL=C $SORT_DIR sort -t'	' -k2,2 $p > $p.sorted) && mv $p.sorted $p"
   fi
done \
| run-parallel.sh - `ls $WORKDIR/ | \wc -l`


if [ -n "$REBUILD_VALID_LM" ]; then
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
      [[ $p =~ ".part([0-9]+)$" ]];
      N=${BASH_REMATCH[1]};
      echo "ngram $N="`\wc -l < $p`;
   done
   echo
   for p in $WORKDIR/part*; do
      [[ $p =~ ".part([0-9]+)$" ]];
      N=${BASH_REMATCH[1]};
      echo "\\$N-grams:"
      cat $p;
      echo
   done
   echo "\\end\\"
   ) | $CAT > $OUTPUT

   TOTAL_ENTRIES=$(gzip -cqfd $INPUT | egrep -v '^$' | \wc -l)

   # Make sure all entries are still present
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
   LC_ALL=C $SORT_DIR sort -t'	' -k2,2 -m $WORKDIR/part* && rm -rf $WORKDIR
fi
