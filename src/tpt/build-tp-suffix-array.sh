#!/bin/bash
# @file build-tp-suffix-array.sh 
# @brief Construct a tightly packed (memory mapped) suffix array for a 
# (tokenized) text (corpus) file. 
#
# The suffix array facilitates fast search for all occurrences of a phrase of
# interest (of any length) in the corpus. The suffix array is encoded using 
# Uli's Tightly Packed tries.
# 
# @author Darlene Stewart, based on testpt2tppt.sh and Uli Germann's mmsufa.compile.sh
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

echo 'build-tp-suffix-array.sh, (c) 2005-2010, Ulrich Germann and Her Majesty in Right of Canada' >&2

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
   cat <<==EOF== >&2

Usage: build-tp-suffix-array.sh <Corpus_filename> [TPSA_prefix]

  Construct a tightly packed (memory mapped) suffix array for a 
  (tokenized) text (corpus) file.
  
  The suffix array facilitates fast search for all occurrences of a phrase of
  interest (of any length) in the corpus. The suffix array is encoded using 
  Uli's Tightly Packed tries.

  The input corpus file may be compressed with gzip (must have .gz suffix).

  Output: directory TPSA_prefix.tpsa containing several files.  Must be in
  the current directory.  [TPSA_prefix is the base name of Corpus_filename]

Options:

  -h(elp)      print this help message

==EOF==

   exit 1
}

VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

set -o errexit

CORPUS=$1
if [[ $2 ]]; then
   OUTPUT_TPSA=$2
   if [[ `basename $OUTPUT_TPSA` != $OUTPUT_TPSA ]]; then
      error_exit "Output model must be created in current directory, without path specification."
   fi
else
   OUTPUT_TPSA=`basename $CORPUS .gz`
fi

OUTPUT_TPSA=`basename $OUTPUT_TPSA .tpsa`

if [[ ! -r $CORPUS ]]; then
   error_exit "Can't read $CORPUS."
fi

echo Building suffix array $OUTPUT_TPSA.tpsa from corpus $TEXT

if [[ `basename $CORPUS` == $CORPUS ]]; then
   #CORPUS is in current directory
   CORPUS=../$CORPUS
elif [[ $CORPUS =~ ^/ ]]; then
   #CORPUS is an absolute path
   true;
else
   #CORPUS is a relative path
   CORPUS=`pwd`/$CORPUS
fi

mkdir -p $OUTPUT_TPSA.tpsa ||
   error_exit "Can't create output dir $OUTPUT_TPSA.tpsa, giving up."

TMPDIR=`mktemp -d $OUTPUT_TPSA.tpsa.tmp.XXX` || error_exit "Cannot create temp workdir."
cd $TMPDIR || error_exit "Can't cd into $TMPDIR, giving up."

if [[ ! -r $CORPUS ]]; then
   error_exit "Can't read $CORPUS."
fi

BIN=`dirname $0`
if [[ $BIN =~ ^\\. ]]; then
   #BIN is a relative path
   BIN=../$BIN
fi

if [ $VERBOSE -eq 0 ]; then
   QUIET="-q"
fi

if [[ $CORPUS =~ \\.gz$ ]]; then
   CAT="zcat -c"
elif [[ $CORPUS =~ \\.bz2$ ]]; then
   CAT="bzcat -c"
else
   CAT="cat"
fi
verbose 1 "$CAT $CORPUS | vocab.build $QUIET --tdx $OUTPUT_TPSA.tdx"
$CAT $CORPUS | time-mem vocab.build $QUIET --tdx $OUTPUT_TPSA.tdx >&2
verbose 1 "$CAT $CORPUS | mmctrack.build $QUIET $OUTPUT_TPSA.tdx $OUTPUT_TPSA.mct"
$CAT $CORPUS | time-mem mmctrack.build $QUIET $OUTPUT_TPSA.tdx $OUTPUT_TPSA.mct >&2
verbose 1 "mmsufa.build $QUIET $OUTPUT_TPSA.mct $OUTPUT_TPSA.msa"
time-mem mmsufa.build $QUIET $OUTPUT_TPSA.mct $OUTPUT_TPSA.msa >&2

for x in tdx mct msa; do
   mv $OUTPUT_TPSA.$x ../$OUTPUT_TPSA.tpsa/$x ||
      error_exit "Can't mv $OUTPUT_TPSA.$x into $OUTPUT_TPSA.tpsa/$x, model probably exists but can't be moved or renamed properly."
done
cd ..
rm -r $TMPDIR

echo "
The three files $OUTPUT_TPSA.tpsa/tdx, $OUTPUT_TPSA.tpsa/mct, and $OUTPUT_TPSA.tpsa/msa,
together, constitute a single tightly packed suffix array model for the
corpus text $CORPUS

For the model to work properly, you should keep the three files together in a
directory called $OUTPUT_TPSA.tpsa

All three files are needed. They cannot be used compressed.

The three files comprising the suffix array model or the corpus text are:
$OUTPUT_TPSA.tpsa/tdx: token index (memory-mapped vocabulary file)
$OUTPUT_TPSA.tpsa/mct: memory-mapped corpus track
$OUTPUT_TPSA.tpsa/msa: memory-mapped suffix array
" > $OUTPUT_TPSA.tpsa/README

echo "ALL DONE!"
if [ $VERBOSE -gt 1 ]; then
   cat $OUTPUT_TPSA.tpsa/README
fi
