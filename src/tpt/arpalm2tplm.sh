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

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

echo 'arpalm2tplm.sh, (c) 2005-2010, Ulrich Germann and Her Majesty in Right of Canada' >&2

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
   -d(ebug)     Debug mode - don't delete tmp directory.

==EOF==

   exit 1
}

MON_PERIOD=
RP_OPTS=
TIME_MEM=
#MON_PERIOD="-period 10"
#RP_OPTS="${MON_PERIOD}"
#TIME_MEM="time-mem ${MON_PERIOD}"

redirect()
{
   if [[ $TIME_MEM ]]; then
      echo "../log.${OUTPUTLM}.$1"
   else
      echo "2"
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

NUM_PAR=1
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)     usage;;
   -n)           arg_check 1 $# $!; arg_check_pos_int $2 $1; NUM_PAR=$2; shift;;
   -v|-verbose)  VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)    DEBUG=1;;
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

echo "Building TPLM $OUTPUTLM.tplm of order $LMORDER for LM $TEXTLM, using $NUM_PAR parallel job(s)." >&2

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

if [[ $DEBUG ]]; then
   echo "Using:" >&2
   which arpalm.encode >&2
   which arpalm.sng-av >&2
   which arpalm.assemble >&2
fi

verbose 1 "Start encoding."
verbose 1 "arpalm.encode $TEXTLM ../$OUTPUTLM.tplm/ >& $(redirect arpalm.encode)"
${TIME_MEM} arpalm.encode $TEXTLM ../$OUTPUTLM.tplm/ >& $(redirect arpalm.encode)
perl -nle '@tokens = split; print "@tokens &> log.$tokens[-1]"' \
   < sng-av.jobs > sng-av.jobs.logging

verbose 1 "Running sub jobs in parallel."
verbose 1 "run-parallel.sh ${RP_OPTS} sng-av.jobs.logging $NUM_PAR >& $(redirect arpalm.sng-av)"
run-parallel.sh ${RP_OPTS} sng-av.jobs.logging $NUM_PAR >& $(redirect arpalm.sng-av)

verbose 1 "Assembling sub-jobs."
verbose 1 "arpalm.assemble $LMORDER ../$OUTPUTLM.tplm/ >& $(redirect arpalm.assemble)"
${TIME_MEM} arpalm.assemble $LMORDER ../$OUTPUTLM.tplm/ >& $(redirect arpalm.assemble)
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
[[ ! $DEBUG ]] && rm -r $TMPDIR

verbose 1 "DONE"
