#!/bin/bash
# @file arpalm2tplm.sh 
# @brief Convert an ARPA format LM into a Tightly Packed LM (TPLM): an LM
# encoded using Uli's Tightly Packed tries.
# 
# @author Eric Joanis, based on Uli Germann's usage instructions
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada


usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: arpalm2tplm.sh [-n NUM_PAR] <ARPALM_filename> [TPLM_prefix]

   Convert an ARPA format LM into a Tightly Packed LM (TPLM):
   an LM encoded using Uli Germann's Tightly Packed tries.

   Output: directory TPLM_prefix.tplm containing several files.  Must be in
   the current directory.  [TPLM_prefix is the base name of ARPALM_filename]

Options:

   -n NUM_PAR   Specifies how many parallel workers to use [4]
   -h(elp)      Print this help message
   -v(erbose)   Increment the verbosity level by 1 (may be repeated)

==EOF==

   exit 1
}

# error_exit "some error message" "optionnally a second line of error message"
# will exit with an error status, print the specified error message(s) on
# STDERR.
error_exit() {
   echo -n arpalm2tplm.sh: "" >&2
   for msg in "$@"; do
      echo $msg >&2
   done
   echo "Use -h for help." >&2
   exit 1
}

# Verify that enough args remain on the command line
# syntax: one_arg_check <args needed> $# <arg name>
# Note that the syntax show above is meant to be part of a while/case structure
# for handling parameters, so that $# still includes the option itself.  exits
# with error message if the check fails.
arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

# arg_check_int $value $arg_name exits with an error if $value does not
# represent an integer, using $arg_name to provide a meaningful error message.
check_pos_int() {
   expr "$1" + 0 &> /dev/null
   RC=$?
   if [ $RC != 0 -a $RC != 1 ] || [ $1 -le 0 ]; then
      error_exit "Invalid $2: $1; positive integer expected."
   fi
}

# Print a verbose message.
verbose() {
   level=$1; shift
   if [[ $level -le $VERBOSE ]]; then
      echo "$*" >&2
   fi
}

NUM_PAR=1
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)     usage;;
   -n)           arg_check 1 $# $!; check_pos_int $2 $1; NUM_PAR=$2; shift;;
   -v|-verbose)  VERBOSE=$(( $VERBOSE + 1 ));;
   --)           shift; break;;
   -*)           error_exit "Unknown option $1.";;
   *)            break;;
   esac
   shift
done

test $# -lt 1 && error_exit "Missing mandatory argument: ARPALM filename."
test $# -gt 2 && error_exit "Too many arguments."

TEXTLM=$1
if [[ ! -r $TEXTLM ]]; then
   error_exit "Can't read $TEXTLM."
fi

LMORDER=`lm-order.pl $TEXTLM`
check_pos_int $LMORDER "LM order for $TEXTLM"

if [[ $2 ]]; then
   OUTPUTLM=$2
   if [[ `basename $OUTPUTLM` != $OUTPUTLM ]]; then
      error_exit "Output model must be created in current directory, without path specification."
   fi
else
   OUTPUTLM=`basename $TEXTLM .gz`
fi

OUTPUTLM=`basename $OUTPUTLM .tplm`

echo "Building TPLM $OUTPUTLM.tplm of order $LMORDER for LM $TEXTLM, using $NUM_PAR parallel job(s)."

if [[ `basename $TEXTLM` == $TEXTLM ]]; then
   #TEXTLM is in current directory
   TEXTLM=../$TEXTLM
elif [[ $TEXTLM =~ ^/ ]]; then
   #TEXTLM is an absolute path
   true;
else
   #TEXTLM is a relative path
   TEXTLM=`pwd`/$TEXTLM
fi

set -o errexit

mkdir -p $OUTPUTLM.tplm ||
   error_exit "Can't create output dir $OUTPUTLM.tplm, giving up."
TMPDIR=$OUTPUTLM.tplm.tmp.$$
mkdir $TMPDIR || error_exit "Can't create temp workdir, giving up."
cd $TMPDIR

if [[ ! -r $TEXTLM ]]; then
   cd ..
   rmdir $TMPDIR || true
   error_exit "Can't read $TEXTLM."
fi

verbose 1 "Start encoding."
verbose 1 "arpalm.encode $TEXTLM ../$OUTPUTLM.tplm/"
arpalm.encode $TEXTLM ../$OUTPUTLM.tplm/ >&2
perl -nle '@tokens = split; print "@tokens &> log.$tokens[-1]"' \
   < sng-av.jobs > sng-av.jobs.logging

verbose 1 "Running sub jobs in parallel."
verbose 1 "run-parallel.sh sng-av.jobs.logging $NUM_PAR"
run-parallel.sh sng-av.jobs.logging $NUM_PAR >&2

verbose 1 "Assembling sub-jobs."
verbose 1 "arpalm.assemble $LMORDER ../$OUTPUTLM.tplm/"
arpalm.assemble $LMORDER ../$OUTPUTLM.tplm/ >&2
cd ..

for x in cbk tdx tplm; do
   mv $OUTPUTLM.tplm/.$x $OUTPUTLM.tplm/$x
done

verbose 1 "Writing the README."
echo "
The three files tplm, cbk and tdx, together, constitute a single TPLM model.
You must keep them together in a directory called NAME.tplm for the model to
work properly.  They cannot be used compressed.

To use this model in any Portage program, provide the directory name NAME.tplm
anywhere you would normally provide an LM file name.
" > $OUTPUTLM.tplm/README

verbose 1 "Cleaning up!"
rm -r $TMPDIR

verbose 1 "DONE"