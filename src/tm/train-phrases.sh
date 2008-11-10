#!/bin/bash
# $Id$

# @file train-phrases.sh 
# @brief Run the translation model training suite.
# 
# @author Eric Joanis, based on George Foster's similar tcsh script
# 
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

echo 'train-phrases.sh, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada'

usage() {
    for msg in "$@"; do
        echo $msg >&2
    done
    cat <<==EOF== >&2

Usage: train-phrases.sh [-h(elp)][-p][-a][-mN][-no-z][-no-bin] dir

  Train a phrase-based model (+ IBM models) on a parallel corpus in directory
  <dir>. The corpus should consist of line-aligned files of the form
  file_{lang1,lang2}.al, where lang1 and lang2 are language abbreviations. The
  models and log files are put in the current directory (which may be the same
  as <dir>).

Options:

  -p     Train IBM models in both directions in parallel. Don't do this for big
         corpora!
  -a     Produce individual phrase tables, voc files, and ngram files for
         adaptation.
  -mN    Override the phrase length parameter to gen_phrase_tables [-m8]
  -hmm   Train HMM models for phrase extraction [IBM2]
  -no-z  Don't compress output phrase tables [do]
  -no-bin  Generate IBM models in non-binary format [generate bin ttables]
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
adapt=
m_opt=-m8
hmm=""
zflag="-z"
bin="-bin"

while [ $# -gt 0 ]; do
    case "$1" in
    -p)             parallel=1;;
    -a)             adapt=-i;;
    -m*)            m_opt=$1;;
    -hmm)           hmm="-hmm";;
    -no-z)          zflag="";;
    -no-bin)        bin="";;
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

echo "WARNING: train-phrases.sh is obsolete." >&2
echo "Please use the methods shown in the sample framework instead." >&2

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

if [ $hmm ]; then
    model="hmm"
else
    model="ibm2"
fi

if [ $parallel ]; then
   echo training parallel
   run "train_ibm -v  $bin $hmm -s ibm1.${l2}_given_${l1} ${model}.${l2}_given_${l1} $dir/*.al >& log.train_ibm.${l2}_given_${l1} &"
   run "train_ibm -vr $bin $hmm -s ibm1.${l1}_given_${l2} ${model}.${l1}_given_${l2} $dir/*.al >& log.train_ibm.${l1}_given_${l2} &"
   wait
else
   echo training not parallel
   run "train_ibm -v  $bin $hmm -s ibm1.${l2}_given_${l1} ${model}.${l2}_given_${l1} $dir/*.al >& log.train_ibm.${l2}_given_${l1}"
   run "train_ibm -vr $bin $hmm -s ibm1.${l1}_given_${l2} ${model}.${l1}_given_${l2} $dir/*.al >& log.train_ibm.${l1}_given_${l2}"
fi

echo

run "gen_phrase_tables -v $adapt -multipr both -w1 $m_opt -1 $l1 -2 $l2 $zflag \
   ${model}.${l2}_given_${l1} ${model}.${l1}_given_${l2} $dir/*.al \
   >& log.gen_phrase_tables"

echo

if [ $adapt ]; then
   for f in $dir/*.al; do
      run "get_voc -c $f > `basename $f .al`.voc"
      run "ngram-count -text $f -write `basename $f .al`.ng"
   done
fi

