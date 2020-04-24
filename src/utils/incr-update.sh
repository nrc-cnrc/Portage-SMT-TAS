#!/bin/bash
# @file incr-update.sh
# @brief take a small corpus of sentence pairs and update the incremental model with it
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, 2020, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, 2020, Her Majesty in Right of Canada

set -o errexit

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   BIN="$BIN/utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }
print_nrc_copyright incremental-update.sh 2017
export PORTAGE_INTERNAL_CALL=1

run_cmd() {
   echo "=======> START [`date`]: $*" >&2
   if [[ ! $NOTREALLY ]]; then
      # Turn off the errexit option temporarily, to facilitate echoing the
      # completion time/exit status of the command.
      local errexit=$(set -o | grep errexit | cut -f2)
      set +o errexit
      eval time $*
      rc=$?
      echo "== DONE (rc=$rc) [`date`]: $*" >&2
      echo >&2
      [[ $errexit = off ]] || set -o errexit
      return $rc
   fi
}

# Make sure local plugins are visible to this script
export PATH=./plugins:$PATH

INCREMENTAL_TM_BASE=cpt.incremental
INCREMENTAL_LM_BASE=lm.incremental
ALIGNMENT_MODEL_BASE=models/tm/hmm3.tm-train.
incr_canoe_ini=canoe.ini.cow
local_canoe_ini=${incr_canoe_ini}.orig

function usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG [options] [-c CANOE_INI] INCREMENTAL_CORPUS [SRC_BLOCK_CORPUS TGT_BLOCK_CORPUS]

  Retrain the incremental model for a PortageLive module, given the
  incremental corpus INCREMENTAL_CORPUS which must contain one sentence pair
  per line in the format:
     DATE <tab> SOURCE SENTENCE <tab> TARGET SENTENCE [<tab> EXTRA-DATA ]

  The directory containing CANOE_INI is also expected to contain an incremental
  config file, incremental.config, with the following contents:
    Format: bash variable definitions
    Expected variables:
      SRC_LANG
      TGT_LANG
      INCREMENTAL_TM_BASE  [$INCREMENTAL_TM_BASE]
      INCREMENTAL_LM_BASE  [$INCREMENTAL_LM_BASE]
      ALIGNMENT_MODEL_BASE [$ALIGNMENT_MODEL_BASE]
      {SRC,TGT}_PREPROCESS_CMD [preprocess_plugin]
      {SRC,TGT}_TOKENIZE_CMD [utokenize.pl -noss, or tokenize_plugin]
      {SRC,TGT}_LOWERCASE_CMD [utf8_casemap -c l]
      {SRC,TGT}_SENTSPLIT_CMD [utokenize.pl -notok -ss -paraline -p, or sentsplit_plugin]

  SRC_LANG and TGT_LANG are used to determine how to tokenize the corpus and
  must be specified in incremental.config.

  If CANOE_INI is not specified, then the incremental model must already be
  initialized in the current directory, and the name of the incremental
  canoe.ini is assumed to be canoe.ini.cow.
  
  If CANOE_INI is specified and the incremental model has already been
  initialized in the current directory, then it is not re-initialized and the
  name of the incremental canoe.ini is assumed to be the basename of CANOE_INI.

  Block-mode update:
  If SRC_BLOCK_CORPUS and TGT_BLOCK_CORPUS are specified, blocks of parallel
  text contained in that file pair, separated by __BLOCK_END__, will be aligned
  using ssal-pipeline.sh and added to INCREMENTAL_CORPUS before updating the
  models. The two files will be deleted once processed. If they are empty or
  non-existent, the update will simply proceed with the text in
  INCREMENTAL_CORPUS. SRC_BLOCK_CORPUS and TGT_BLOCK_CORPUS must both contain
  exactly the same number of lines containing just __BLOCK_END__.

Options:

  -c CANOE_INI a canoe.ini to build incremental on-top of

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

DEBUG=
DEBUG_OPT=
VERBOSE=1
while [ $# -gt 0 ]; do
   case "$1" in
   -c)                  arg_check 1 $# $1; readonly base_canoe_ini=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -q|-quiet)           VERBOSE=0;;
   -d|-debug)           DEBUG=1; DEBUG_OPT="-debug";;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

[[ $# -eq 0 ]]  && error_exit "Missing INCREMENTAL_CORPUS argument"
readonly INCREMENTAL_CORPUS=$1; shift
if [[ $# -eq 2 ]]; then
   SRC_BLOCK_CORPUS=$1
   TGT_BLOCK_CORPUS=$2
   shift; shift
fi
[[ $# -gt 0 ]]  && error_exit "Superfluous arguments $*"

# Initialize the incremental model, if needed.
init_model_opts=
if [[ -n "$base_canoe_ini" ]]; then
   [[ $VERBOSE -gt 0 ]] && init_model_opts+=" -v"
   run_cmd "incr-init-model.py -q $init_model_opts -- $base_canoe_ini" ||
      error_exit "Cannot initialize document model (RC=$rc)."
   incr_canoe_ini=$(basename $base_canoe_ini)
   local_canoe_ini=${incr_canoe_ini}.orig
fi

CONFIG_FILE=incremental.config

if [[ $CONFIG_FILE ]]; then
   [[ -r $CONFIG_FILE ]] || error_exit "Cannot read config file $CONFIG_FILE"
   source $CONFIG_FILE || error_exit "Error reading config file $CONFIG_FILE"
fi

[[ $SRC_LANG ]] || error_exit "SRC_LANG not defined, specify in $CONFIG_FILE"
[[ $TGT_LANG ]] || error_exit "TGT_LANG not defined, specify in $CONFIG_FILE"

[[ $SRC_PREPROCESS_CMD ]] || SRC_PREPROCESS_CMD="preprocess_plugin $SRC_LANG"
[[ $TGT_PREPROCESS_CMD ]] || TGT_PREPROCESS_CMD="preprocess_plugin $TGT_LANG"
[[ $SRC_LOWERCASE_CMD ]] || SRC_LOWERCASE_CMD="utf8_casemap -c l"
[[ $TGT_LOWERCASE_CMD ]] || TGT_LOWERCASE_CMD="utf8_casemap -c l"
if [[ ! $SRC_SENTSPLIT_CMD ]]; then
   if utokenize.pl -notok -ss -paraline -p -lang=$SRC_LANG /dev/null >& /dev/null; then
      SRC_SENTSPLIT_CMD="utokenize.pl -notok -ss -paraline -p -lang=$SRC_LANG"
   else
      SRC_SENTSPLIT_CMD="sentsplit_plugin $SRC_LANG"
   fi
fi
if [[ ! $TGT_SENTSPLIT_CMD ]]; then
   if utokenize.pl -notok -ss -paraline -p -lang=$TGT_LANG /dev/null >& /dev/null; then
      TGT_SENTSPLIT_CMD="utokenize.pl -notok -ss -paraline -p -lang=$TGT_LANG"
   else
      TGT_SENTSPLIT_CMD="sentsplit_plugin $TGT_LANG"
   fi
fi
if [[ ! $SRC_TOKENIZE_CMD ]]; then
   if utokenize.pl -noss -lang $SRC_LANG /dev/null >& /dev/null; then
      SRC_TOKENIZE_CMD="utokenize.pl -noss -lang $SRC_LANG"
   else
      SRC_TOKENIZE_CMD="tokenize_plugin $SRC_LANG"
   fi
fi
if [[ ! $TGT_TOKENIZE_CMD ]]; then
   if utokenize.pl -noss -lang $TGT_LANG /dev/null >& /dev/null; then
      TGT_TOKENIZE_CMD="utokenize.pl -noss -lang $TGT_LANG"
   else
      TGT_TOKENIZE_CMD="tokenize_plugin $TGT_LANG"
   fi
fi

[[ $MAX_INCR_CORPUS_SIZE ]] || MAX_INCR_CORPUS_SIZE=2000

# Don't try to make any models if there is nothing in the corpora.
if ! [[ -s $INCREMENTAL_CORPUS || -s $SRC_BLOCK_CORPUS || -s $TGT_BLOCK_CORPUS ]]; then
   warn "No corpora, nothing to do."
   exit 0
fi

INCREMENTAL_TM=$INCREMENTAL_TM_BASE.${SRC_LANG}2$TGT_LANG
INCREMENTAL_LM=${INCREMENTAL_LM_BASE}_$TGT_LANG

WD=`mktemp -d incremental-tmp.XXX` ||
   error_exit "Cannot create temporary working directory"
verbose 1 Created working directory $WD

if [[ -s $SRC_BLOCK_CORPUS && -s $TGT_BLOCK_CORPUS ]]; then
   # Sentence align the incremental blocks before using the corpus
   # Instead of creating an ssal config file, export the configuration variables it needs
   pushd $WD
   echo "
L1_CLEAN=\"$SRC_PREPROCESS_CMD\"
L2_CLEAN=\"$TGT_PREPROCESS_CMD\"
L1_SS=\"$SRC_SENTSPLIT_CMD\"
L2_SS=\"$TGT_SENTSPLIT_CMD\"
L1_TOK=\"$SRC_TOKENIZE_CMD\"
L2_TOK=\"$TGT_TOKENIZE_CMD\"
IBM_L1_GIVEN_L2=\"../$ALIGNMENT_MODEL_BASE${SRC_LANG}_given_$TGT_LANG.gz\"
IBM_L2_GIVEN_L1=\"../$ALIGNMENT_MODEL_BASE${TGT_LANG}_given_$SRC_LANG.gz\"
" > ssal-config
   SRC_AL_OUT=src-blocks.al
   TGT_AL_OUT=tgt-blocks.al
   VERBOSE_FLAGS=$(for x in $(seq 1 $VERBOSE); do echo -n " -v"; done)
   run_cmd "ssal-pipeline.sh $VERBOSE_FLAGS -c ssal-config $DEBUG_OPT -hm __BLOCK_END__ \
            ../$SRC_BLOCK_CORPUS ../$TGT_BLOCK_CORPUS \
            $SRC_AL_OUT $TGT_AL_OUT"
   popd
   verbose 1 "Appending aligned block corpus to $INCREMENTAL_CORPUS."
   paste /dev/null $WD/$SRC_AL_OUT $WD/$TGT_AL_OUT /dev/null |
      grep -v $'\t\t' | # remove sentence pairs where either side is empty
      sed "s/^/$(date)/" | # add the date we're adding the sentence pairs
      cat >> $INCREMENTAL_CORPUS
   verbose 1 "Removing block corpus now that it is in sentence pair corpus."
   run_cmd "rm $SRC_BLOCK_CORPUS $TGT_BLOCK_CORPUS"
fi

CORPUS_SIZE=`wc -l < $INCREMENTAL_CORPUS`
if [[ $CORPUS_SIZE -eq 0 ]]; then
   warn "Empty incremental corpus, nothing to do."
   exit 0
fi

# Apply the rolling window
if [[ $CORPUS_SIZE -gt $MAX_INCR_CORPUS_SIZE ]]; then
   TMP_CORPUS=`mktemp $INCREMENTAL_CORPUS.truncated.XXX`
   verbose 1 Truncating corpus via $TMP_CORPUS to max size $MAX_INCR_CORPUS_SIZE
   run_cmd "tail -$MAX_INCR_CORPUS_SIZE < $INCREMENTAL_CORPUS > $TMP_CORPUS"
   run_cmd "mv $TMP_CORPUS $INCREMENTAL_CORPUS"
fi

# Separate the tab-separated corpus file into clean OSPL source and target files
# Warning: don't call clean-utf8-text.pl before cut, since it replaces tab characters by spaces.
verbose 1 Split the corpus into source and target
run_cmd "cut -f 2 $INCREMENTAL_CORPUS | $SRC_PREPROCESS_CMD > $WD/source.raw"
run_cmd "cut -f 3 $INCREMENTAL_CORPUS | $TGT_PREPROCESS_CMD > $WD/target.raw"

# Tokenize
verbose 1 Tokenize the source and target
run_cmd "$SRC_TOKENIZE_CMD < $WD/source.raw > $WD/source.tok"
run_cmd "$TGT_TOKENIZE_CMD < $WD/target.raw > $WD/target.tok"

# Lowercase
verbose 1 Lowercase source and target
run_cmd "$SRC_LOWERCASE_CMD < $WD/source.tok > $WD/source.lc"
run_cmd "$TGT_LOWERCASE_CMD < $WD/target.tok > $WD/target.lc"

# LM
verbose 1 Train the incremental LM on target
set +o errexit
ulimit -c 0 # suppress estimate-ngram's core dump files (when corpus is too small)
ESTIMATE_FILTER="perl -ple 's/^\\s+//; s/\\s+\$//; s/\\s+/ /g;' | fold -s -w 4095"
ESTIMATE_CMD="estimate-ngram -s ML -text /dev/stdin -write-lm $WD/$INCREMENTAL_LM"
run_cmd "cat $WD/target.lc | $ESTIMATE_FILTER | $ESTIMATE_CMD"
RC=$?
set -o errexit
#echo RC=$RC
if [[ $RC == 139 ]]; then
   echo "Warning: estimate-ngram core dumped; adding dummy tokens to the document LM corpus and trying again"
   { echo __DUMMY__ __DUMMY__ __DUMMY__ __DUMMY__; cat $WD/target.lc; } > $WD/target.lc.forLM
   run_cmd "cat $WD/target.lc.forLM | $ESTIMATE_FILTER | $ESTIMATE_CMD" ||
      error_exit "Cannot train document LM with added dummy tokens"
elif [[ $RC != 0 ]]; then
   error_exit "Cannot train document LM"
fi
verbose 1 Tightly pack the incremental LM
run_cmd "(cd $WD; arpalm2tplm.sh $INCREMENTAL_LM)" ||
   error_exit "Cannot tightly pack document LM"

# TM
verbose 1 Train the incremental TM from source and target
run_cmd "gen_phrase_tables -o $WD/$INCREMENTAL_TM_BASE -1 $SRC_LANG -2 $TGT_LANG -multipr fwd \
   -s RFSmoother -s ZNSmoother -write-count -write-al top -whole -m 8 -a GDFA \
   $ALIGNMENT_MODEL_BASE${TGT_LANG}_given_$SRC_LANG.gz \
   $ALIGNMENT_MODEL_BASE${SRC_LANG}_given_$TGT_LANG.gz \
   $WD/source.lc $WD/target.lc" ||
   error_exit "Cannot generated document TM"

if [[ ! -s $WD/$INCREMENTAL_TM ]]; then
   verbose 1 Generated TM is empty, putting a dummy phrase pair in it
   echo '__DUMMY__ ||| __DUMMY__ ||| 1 1.17549e-38 1 1.17549e-38 a=0 c=1' > $WD/$INCREMENTAL_TM
fi

ls $WD
verbose 1 Tightly pack the incremental TM
run_cmd "(cd $WD; textpt2tppt.sh $INCREMENTAL_TM)" ||
   error_exit "Cannot tightly pack document TM"

(
   # Get the lock to update the models
   verbose 1 "Get the update lock `date`"
   flock --exclusive 202
   verbose 1 "Got the update lock `date`"

   readonly BK=incremental-backup
   rm -rf $BK
   mkdir $BK
   #ls -l $WD $BK .
   time for model in $INCREMENTAL_TM $INCREMENTAL_TM.tppt $INCREMENTAL_LM $INCREMENTAL_LM.tplm; do
      if [[ -e $model ]]; then
         run_cmd "mv $model $BK"
      fi
      run_cmd "mv $WD/$model ."
   done
   #ls -l $WD $BK .

   # validation
   verbose 1 "Checking the final config"
   if run_cmd "configtool check $incr_canoe_ini"; then
      verbose 1 "All good"
   else
      verbose 1 "Problem with final canoe config, rolling back update"
      for model in $INCREMENTAL_TM $INCREMENTAL_TM.tppt $INCREMENTAL_LM $INCREMENTAL_LM.tplm; do
         run_cmd "mv $model $WD" || true
         run_cmd "mv $BK/$model ." || true
      done
      exit 1
   fi

   # sleep 5 # insert this to test whether the locking is really working
   verbose 1 "Releasing update lock"
) 202<$incr_canoe_ini

# Everything worked fine, clean up
[[ $DEBUG ]] || rm -rf $WD
