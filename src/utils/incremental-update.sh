#!/bin/bash
# @file incremental-update.sh
# @brief take a small corpus of sentence pairs and update the incremental model with it
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

set -o errexit

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   BIN="$BIN/utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }
print_nrc_copyright incremental-update.sh 2017
export PORTAGE_INTERNAL_CALL=1

INCREMENTAL_TM_BASE=cpt.incremental
INCREMENTAL_LM_BASE=lm.incremental
ALIGNMENT_MODEL_BASE=models/tm/hmm3.tm-train.
UPDATE_LOCK_FILE=incremental-update.lock
readonly local_canoe_ini=canoe.ini

function usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG [options] [-f CONFIG] [-c CANOE_INI] SRC_LANG TGT_LANG INCREMENTAL_CORPUS

  Retrain the incremental corpus for a PortageLive module, given the
  incremental corpus INCREMENTAL_CORPUS which must contain one sentence pair
  per line in the format:
     DATE <tab> SOURCE SENTENCE <tab> TARGET SENTENCE [<tab> EXTRA-DATA ]

  SRC_LANG and TGT_LANG are used to determine how to tokenize the corpus.

Options:

  -c CANOEINI a canoe.ini to build incremental on-top of.
  -f CONFIG   read parameters from CONFIG.
              Format: bash variable definitions
              Supported/expected variables:
                 INCREMENTAL_TM_BASE  [$INCREMENTAL_TM_BASE]
                 INCREMENTAL_LM_BASE  [$INCREMENTAL_LM_BASE]
                 UPDATE_LOCK_FILE     [$UPDATE_LOCK_FILE]
                 ALIGNMENT_MODEL_BASE [$ALIGNMENT_MODEL_BASE]

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -f)                  arg_check 1 $# $1; CONFIG_FILE=$2; shift;;
   -c)                  arg_check 1 $# $1; readonly base_canoe_ini=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

[[ $# -eq 0 ]]  && error_exit "Missing SRC_LANG argument"
readonly SRC_LANG=$1; shift
test $# -eq 0   && error_exit "Missing TGT_LANG argument"
readonly TGT_LANG=$1; shift
test $# -eq 0   && error_exit "Missing INCREMENTAL_CORPUS argument"
readonly INCREMENTAL_CORPUS=$1; shift
test $# -gt 0   && error_exit "Superfluous arguments $*"

if [[ $CONFIG_FILE ]]; then
   [[ -r $CONFIG_FILE ]] || error_exit "Cannot read config file $CONFIG_FILE"
   source $CONFIG_FILE || error_exit "Error reading config file $CONFIG_FILE"
fi


function initialize_incremental_directory() {
   local readonly base_canoe_ini=$1

   # Setup the model directory and the canoe.ini.
   [[ -r $base_canoe_ini ]] || error_exit "Cannot read the CANOE_INI file $base_canoe_ini"

   local readonly base_dir=`basename $base_canoe_ini`
   [[ -s $local_canoe_ini ]] || cp $base_canoe_ini $local_canoe_ini
   [[ -L "prime.sh" ]] || ln -s $base_dir/prime.sh .
   [[ -L "soap-translate.sh" ]] || ln -s $base_dir/soap-translate.sh .
   [[ -L models ]] || ln -s $base_dir/models models
   if [[ -d "$base_dir/plugins" ]]; then
      [[ -L plugins ]] || ln -s $base_dir/plugins .
   fi
}

[[ -n "$base_canoe_ini" ]] && initialize_incremental_directory $base_canoe_ini

# Don't try to make any models if there is nothing in the corpora.
[[ `wc -l < $INCREMENTAL_CORPUS` -gt 0 ]] || exit 0

UPDATE_LOCK_FD=202
INCREMENTAL_TM=$INCREMENTAL_TM_BASE.${SRC_LANG}2$TGT_LANG
INCREMENTAL_LM=${INCREMENTAL_LM_BASE}_$TGT_LANG

WD=`mktemp -d incremental-tmp.XXX` ||
   error_exit "Cannot create temporary working directory"
verbose 1 Created working directory $WD

# Separate the tab-separated corpus file into clean OSPL source and target files
# Warning: don't call clean-utf8-text.pl before cut, since it replaces tab characters by spaces.
verbose 1 Split the corpus into source and target
time cut -f 2 $INCREMENTAL_CORPUS | clean-utf8-text.pl > $WD/source.raw
time cut -f 3 $INCREMENTAL_CORPUS | clean-utf8-text.pl > $WD/target.raw

# Tokenize -- TODO test the path through tokenize_plugin
verbose 1 Tokenize the source
time utokenize.pl -noss -lang $SRC_LANG < $WD/source.raw > $WD/source.tok ||
   tokenize_plugin $SRC_LANG < $WD/source.raw > $WD/source.tok ||
   error_exit "Cannot tokenize source corpus."

verbose 1 Tokenize the target
time utokenize.pl -noss -lang $TGT_LANG < $WD/target.raw > $WD/target.tok ||
   tokenize_plugin $TGT_LANG < $WD/target.raw > $WD/target.tok ||
   error_exit "Cannot tokenize target corpus."

# Lowercase -- TODO make this step optional; won't always be appropriate
verbose 1 Lowercase source and target
time utf8_casemap -c l < $WD/source.tok > $WD/source.lc
time utf8_casemap -c l < $WD/target.tok > $WD/target.lc

# LM
verbose 1 Train the incremental LM on target
time estimate-ngram -s ML -text $WD/target.lc -write-lm $WD/$INCREMENTAL_LM >& $WD/log.$INCREMENTAL_LM
verbose 1 Tightly pack the incremental LM
time (
   cd $WD
   arpalm2tplm.sh $INCREMENTAL_LM >& log.$INCREMENTAL_LM.tplm
)

# TM
verbose 1 Train the incremental TM from source and target
time gen_phrase_tables -o $WD/$INCREMENTAL_TM_BASE -1 $SRC_LANG -2 $TGT_LANG -multipr fwd \
   -s RFSmoother -s ZNSmoother -write-count -write-al top \
   $ALIGNMENT_MODEL_BASE${TGT_LANG}_given_$SRC_LANG.gz \
   $ALIGNMENT_MODEL_BASE${SRC_LANG}_given_$TGT_LANG.gz \
   $WD/source.lc $WD/target.lc
ls $WD
verbose 1 Tightly pack the incremental TM
time (
   cd $WD
   textpt2tppt.sh $INCREMENTAL_TM >& log.$INCREMENTAL_TM.tppt
)

# Get the lock to update the models
verbose 1 "Get the update lock"
eval "exec $UPDATE_LOCK_FD>$UPDATE_LOCK_FILE"
flock --exclusive $UPDATE_LOCK_FD
readonly BK=incremental-backup
rm -rf $BK
mkdir $BK
#ls -l $WD $BK .
time for model in $INCREMENTAL_TM $INCREMENTAL_TM.tppt $INCREMENTAL_LM $INCREMENTAL_LM.tplm; do
   if [[ -e $model ]]; then
      echo mv $model $BK
      mv $model $BK
   fi
   mv $WD/$model .
done
#ls -l $WD $BK .

# validation
verbose 1 "Checking the final config"
if time configtool check canoe.ini.incr; then
   verbose 1 "All good"
else
   verbose 1 "Problem with final canoe config, rolling back update"
   for model in $INCREMENTAL_TM $INCREMENTAL_TM.tppt $INCREMENTAL_LM $INCREMENTAL_LM.tplm; do
      mv $model $WD || true
      mv $BK/$model . || true
   done
   exit 1
fi

# sleep 5 # insert this to test whether the locking is really working
verbose 1 "Releasing update lock"
flock --unlock $UPDATE_LOCK_FD
