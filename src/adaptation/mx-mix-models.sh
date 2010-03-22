#!/bin/bash

# @file mx-mix-models.sh 
# @brief Generate a mixture model of a specified type, by applying given
# weights to a set of components, for translating a given textfile.
# 
# @author George Foster
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

print_nrc_copyright mx-mix-models.sh 2006
export PORTAGE_INTERNAL_CALL=1

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

mx-mix-models.sh [-v][-nofilt][-a args][-d pfx][-e ext]
                 type wts components textfile

Generate a mixture model of a specified type, by applying given weights to a
set of components, for translating a given textfile. <wts> is a file containing
weights; <components> contains basenames of curresponding models (complete
paths for models are specified by -d and -e). <type> is one of:

   mixlm  - mix language models by writing mixlm file
   rf     - mix relative-frequency phrase tables, using:
            mix_phrasetables args -wf wts -f textfile models

The resulting mixture model is written to stdout.

Options:

-v  increment the verbosity level by 1 (may be repeated)
-nofilt  don't filter models for textfile (if applicable) [filter]
-d  prepend <pfx> to the pathname for component files
-e  append <ext> to the pathname for component files
-a  pass argument(s) <args> to mixing program

==EOF==

    exit 1
}

# Command line processing

VERBOSE=0
cmpt_pfx=
cmpt_ext=
args=
filt="-f"

while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -nofilt)             filt="" ;;
   -d)                  arg_check 1 $# $1; cmpt_pfx=$2; shift;;
   -e)                  arg_check 1 $# $1; cmpt_ext=$2; shift;;
   -a)                  arg_check 1 $# $1; args=$2; shift;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

if [ $# -ne 4 ]; then
   error_exit "Expecting 4 arguments!"
fi

type=$1
wts=$2
components=$3
textfile=$4

# Check arguments

if [ ! -r $wts ]; then error_exit "Can't read file $wts"; fi
if [ ! -r $components ]; then error_exit "Can't read file $components"; fi
if [ ! -r $textfile ]; then error_exit "Can't read file $textfile"; fi

filter_opt=
if [ -n "$filt" ]; then filter_opt="-f $textfile"; fi

tmp=`/usr/bin/uuidgen`
models="models.$tmp"
(
    for m in `cat $components`; do
        echo $cmpt_pfx$m$cmpt_ext
    done
) > $models

# Run

case $type in
    mixlm)
	paste $models $wts ;;
    rf)
	eval mix_phrasetables $args -wf $wts $filter_opt `cat $models` ;;
    *)        
	error_exit "Unknown metric <$metric>!"
esac

# Cleanup 

rm $models
