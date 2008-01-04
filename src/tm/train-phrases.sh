#!/bin/bash
# $Id$

# train-phrases.sh - Run the translation model training suite
# 
# PROGRAMMER: Eric Joanis, based on George Foster's similar tcsh script
# 
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

echo 'train-phrases.sh, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada'

usage() {
    for msg in "$@"; do
        echo $msg >&2
    done
    cat <<==EOF== >&2

Usage: train-phrases.sh [-h(elp)][-p][-a][-mN][-no-z] dir

  Train a phrase-based model (+ IBM models) on a parallel corpus in directory
  <dir>. The corpus should consist of line-aligned files of the form
  file_{lang1,lang2}.al, where lang1 and lang2 are language abbreviations. The
  models and log files are put in the current directory (which may be the same
  as <dir>).

Options:

  -p     Train IBM models in both directions in parallel. Don't do this for big
         corpora!
  -mN    Override the phrase length parameter to gen_phrase_tables [-m8]
  -no-z  Don't compress output phrase tables [do]
  -h     Print this help message.

==EOF==

    exit 1
}

error_exit() {
    for msg in "" "$@"; do
        echo "$msg" >&2
    done
    #echo "Use -h for help." >&2
    usage
}

arg_check() {
    if [ $2 -le $1 ]; then
        error_exit "Missing argument to $3 option."
    fi
}

set -o noclobber
parallel=
m_opt=-m8
zflag="-z"
while [ $# -gt 0 ]; do
    case "$1" in
    -p)             parallel=1;;
    -m*)            m_opt=$1;;
    -no-z)          zflag="";;
    -h|-help)       usage;;
    --)             shift; break;;
    -*)             error_exit "Unknown option $1.";;
    *)              break;;
    esac
    shift
done

#echo \$\*: $*
#echo \$\#: $#
test $# -gt 1 && error_exit "Error: Too many arguments"
test $# -lt 1 && error_exit "Error: Missing argument: directory"

dir=$1
declare -a langs
langs=(`ls -1 $dir/*.al | head -2 | rev | cut -d'.' -f2- | cut -d_ -f1 | rev`)

if [ ${#langs[*]} != 2 -o ${langs[0]} == ${langs[1]} ]; then
   error_exit "Error: dir <$dir> doesn't seem to contain files of the form" \
              "       file_{lang1,lang2}.al"
fi

l1=${langs[0]}
l2=${langs[1]}

echo
echo l1: $l1 l2: $l2
echo

# Echos a command and then runs it
run () {
   cmd="$*"
   echo "$cmd"
   eval $cmd
}

if [ $parallel ]; then
   echo training parallel
   run "train_ibm -v  -s ibm1.${l2}_given_${l1} ibm2.${l2}_given_${l1} $dir/*.al >& log.train_ibm.${l2}_given_${l1} &"
   run "train_ibm -vr -s ibm1.${l1}_given_${l2} ibm2.${l1}_given_${l2} $dir/*.al >& log.train_ibm.${l1}_given_${l2} &"
   wait
else
   echo training not parallel
   run "train_ibm -v  -s ibm1.${l2}_given_${l1} ibm2.${l2}_given_${l1} $dir/*.al >& log.train_ibm.${l2}_given_${l1}"
   run "train_ibm -vr -s ibm1.${l1}_given_${l2} ibm2.${l1}_given_${l2} $dir/*.al >& log.train_ibm.${l1}_given_${l2}"
fi

echo

run "gen_phrase_tables -v -multipr both -w1 $m_opt -1 $l1 -2 $l2 -ibm 2 $zflag \
   ibm2.${l2}_given_${l1} ibm2.${l1}_given_${l2} $dir/*.al \
   >& log.gen_phrase_tables"

echo

