#!/bin/bash
# @file textpt2tppt.sh 
# @brief Convert a text Phrase Table into a TPPT.
# 
# @author Eric Joanis, based on Uli Germann's usage instructions
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

echo 'textpt2tppt.sh, (c) 2005-2010, Ulrich Germann and Her Majesty in Right of Canada' >&2

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: textpt2tppt.sh <TextPT_filename> [TPPT_prefix]

   Convert a text format Phrase Table into a Tightly Packed Phrase (TPPT):
   a PT encoded using Uli Germann's Tightly Packed tries.

   Output: directory TPPT_prefix.tppt containing several files.  Must be in
   the current directory.  [TPPT_prefix is the base name of TextPT_filename]

Options:

   -h(elp)      print this help message
   -d(ebug)     keep the temporary directory when done

==EOF==

   exit 1
}

VERBOSE=1
TPT_EXTENSION=".tppt"
while [ $# -gt 0 ]; do
   case "$1" in
   # Hidden option for textldm2tpldm.sh.
   -type)               arg_check 1 $# $1; TPT_EXTENSION=$2; shift;;

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

TEXTPT=$1
if [[ $2 ]]; then
   OUTPUTPT=$2
   if [[ `basename $OUTPUTPT` != $OUTPUTPT ]]; then
      error_exit "Output model must be created in current directory, without path specification."
   fi
else
   OUTPUTPT=`basename $TEXTPT .gz`
fi

OUTPUTPT=`basename $OUTPUTPT $TPT_EXTENSION`

if [[ ! -r $TEXTPT ]]; then
   error_exit "Can't read $TEXTPT."
fi

echo "Building TPPT $OUTPUTPT$TPT_EXTENSION from PT $TEXTPT" >&2

if [[ `basename $TEXTPT` == $TEXTPT ]]; then
   #TEXTPT is in current directory
   TEXTPT=../$TEXTPT
elif [[ $TEXTPT =~ ^/ ]]; then
   #TEXTPT is an absolute path
   true;
else
   #TEXTPT is a relative path
   TEXTPT=`pwd`/$TEXTPT
fi

mkdir -p $OUTPUTPT$TPT_EXTENSION ||
   error_exit "Can't create output dir $OUTPUTPT$TPT_EXTENSION, giving up."

TMPDIR=`mktemp -d $OUTPUTPT$TPT_EXTENSION.tmp.XXX` || error_exit "Cannot create temp workdir."
run_cmd cd $TMPDIR || error_exit "Can't cd into $TMPDIR, giving up."

if [[ ! -r $TEXTPT ]]; then
   error_exit "Can't read $TEXTPT."
fi

run_cmd "time-mem ptable.encode-phrases $TEXTPT 1 $OUTPUTPT >&2"
run_cmd "time-mem ptable.encode-phrases $TEXTPT 2 $OUTPUTPT >&2"
run_cmd "time-mem ptable.encode-scores $TEXTPT $OUTPUTPT >&2"
run_cmd "time-mem ptable.assemble $OUTPUTPT >&2"
for x in tppt cbk trg.repos.dat src.tdx trg.tdx; do
   mv $OUTPUTPT.$x ../$OUTPUTPT$TPT_EXTENSION/$x ||
      error_exit "Can't mv $OUTPUTPT.$x into $OUTPUTPT$TPT_EXTENSION/$x, model probably exists but can't be moved or renamed properly."
done
if [[ -f $OUTPUTPT.config ]]; then
   mv $OUTPUTPT.config ../$OUTPUTPT$TPT_EXTENSION/config
   USING_V2=1
fi
cd ..
[[ ! $DEBUG ]] && rm -r $TMPDIR

echo "
The five files tppt, cbk, src.tdx, trg.tdx and trg.repos.dat, together,
constitute a single TPPT model.  You must keep them together in a directory
called NAME$TPT_EXTENSION for the model to work properly.  They cannot be used
compressed.

To use this model in canoe, put a line like this is your canoe.ini file:
   [ttable-tppt] NAME$TPT_EXTENSION
" > $OUTPUTPT$TPT_EXTENSION/README

if [[ $USING_V2 ]]; then
   echo "

The additional file config provides the semantic interpretation of the columns
in the TPPT, supporting 4th column scores and count fields.  Support for
alignments is planned but not implemented yet.
" >> $OUTPUTPT$TPT_EXTENSION/README
fi

echo Done textpt2tppt.sh. >&2


