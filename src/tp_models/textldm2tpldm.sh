#!/bin/bash
# vim:nowrap

# @file textldm2tpldm.sh
# @brief Convert a text LDM to a TPLDM.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

echo 'textldm2tpldm.sh, (c) 2005-2010, Ulrich Germann and Her Majesty in Right of Canada' >&2

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG <textldm_filename> [TPLDM_prefix]

  Converts your textldm into a TPLDM: a lexicalized distortion model encoded
  using Uli Germann's Tightly Packed tries.

  Output: directory TPLDM_prefix.tpldm containing several files.  Must be in
  the current directory.  [TPLDM_prefix is the base name of textldm_filename]

Options:

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ))
                        BASE_OPTIONS="$BASE_OPTIONS $1";;
   -d|-debug)           DEBUG=1
                        BASE_OPTIONS="$BASE_OPTIONS $1";;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0   && error_exit "Missing first argument argument"
MODEL=$1; shift
if [[ $1 ]]; then
   PREFIX=$1; shift
   if [[ `basename $PREFIX` != $PREFIX ]]; then
      error_exit "Output  model must be created in current directory, without path specification."
   fi
else
   PREFIX=`basename $MODEL .gz`
fi
test $# -gt 0   && error_exit "Superfluous arguments $*"

EXTENSION=".tpldm"
BACKOFF=${MODEL%%.gz}.bkoff
OUTPUT_TPLDM="${PREFIX}${EXTENSION}"

# Make sure the backoff model is available.
[[ -s "$BACKOFF" ]] || error_exit "Can't find the backoff model: $BACKOFF";


set -o errexit

# To create a tpldm, we will rely on textpt2tppt.sh since the process is the
# same.
textpt2tppt.sh $BASE_OPTIONS -type $EXTENSION $MODEL $PREFIX

# To have a valid TPLDM, we also need the backoff file.
cp $BACKOFF $OUTPUT_TPLDM/bkoff


# Write and README to explain how to use this tpldm model.
echo "
The six files bkoff, tppt, cbk, src.tdx, trg.tdx and trg.repos.dat, together,
constitute a single TPLDM model.  You must keep them together in a directory
called NAME.tpldm for the model to work properly.  They cannot be used
compressed.

To use this model in canoe, put two lines like these is your canoe.ini file:
   [lex-dist-model-file] NAME.tpldm
   [distortion-model] WordDisplacement:back-lex#m:back-lex#s:back-lex#d:fwd-lex#m:fwd-lex#s:fwd-lex#d
And optionally (but recommended)
   [dist-phrase-swap]
   [dist-limit-ext]
or (possibly even better)
   [dist-limit-simple]

" > $OUTPUT_TPLDM/README

echo Done textldm2tpldm.sh. >&2

