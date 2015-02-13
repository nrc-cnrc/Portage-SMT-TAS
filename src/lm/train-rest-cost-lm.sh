#!/bin/bash

# @file train-rest-cost-lm.sh
# @brief Train an LM using the rest-cost LM training procedure
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

set -o errexit
set -o pipefail

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

# Change the program name and year here
print_nrc_copyright train-rest-cost-lm.sh 2014

run_cmd() {
   cmd=$*
   if [[ $NOTREALLY ]]; then
      verbose 0 "[Exec] $cmd"
   else
      verbose 1 "[`date`] $cmd"
      eval $cmd
      return $?
   fi
}

# Default options
ORDER=5
TOOLKIT=mitlm
MITLM_CMD="estimate-ngram -smoothing ModKN"
SRILM_CMD="ngram-count -kndiscount -interpolate -sort"

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG [options] -text TEXT -lm LM.rclm

  Train an LM using the Rest Cost LM training procedure for KN smoothed LMs.
  This is based mostly on Heafield, Koehn and Lavie, Language Model Rest Costs
  and Space-Efficient Storage, EMNLP 2012, following their Better Rest Costs
  work (section 3.1), but not their Less Memory work (section 3.2).

  The output is two LM files in directory LM.rclm, containing a config
  file, a regular LM and a rest-cost component LM. Use LM.rclm as you
  would any other LM in canoe, lm_eval and other Portage programs accepting
  LMs. The components are converted to TPLMs automatically.

Options:

  -order N     LM order [$ORDER]
  -toolkit T   which toolkit to use: srilm or mitlm [$TOOLKIT]
  -cmd CMD     LM training command (excluding text input and lm output options)
               [if -toolkit mitlm: $MITLM_CMD]
               [if -toolkit srilm: $SRILM_CMD]
  -text TEXT   Training text (-text argument to LM training command) (required)
  -lm LMOUT    Name of the output LM (must end in .rclm; required)

  -v(erbose)   Increment verbosity
  -q(uiet)     Zero-verbosity
  -n(otreally) Do nothing; just show what would be done
  -h(elp)      Display this help message

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=1
while [ $# -gt 0 ]; do
   case "$1" in
   -order)              arg_check 1 $# $1; arg_check_pos_int $2 $1; ORDER=$2; shift;;
   -toolkit)            arg_check 1 $# $1; TOOLKIT=$2; shift;;
   -cmd)                arg_check 1 $# $1; USER_CMD="$2"; shift;;
   -text)               arg_check 1 $# $1; TEXT=$2; shift;;
   -lm)                 arg_check 1 $# $1; LMOUT=$2; shift;;
   -n|-notreally)       NOTREALLY=1;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -q|-quiet)           VERBOSE=0;;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

[[ $TEXT ]] || error_exit "Missing required -text switch"
[[ -r $TEXT ]] || error_exit "Cannot read training corpus $TEXT"
[[ $LMOUT ]] || error_exit "Missing required -lm switch"

if [[ $USER_CMD ]]; then
   TRAIN_CMD=$USER_CMD
   TOOLKIT=
elif [[ $TOOLKIT == mitlm ]]; then
   TRAIN_CMD=$MITLM_CMD
elif [[ $TOOLKIT == srilm ]]; then
   TRAIN_CMD=$SRILM_CMD
else
   error_exit "Unknown LM toolkit $TOOLKIT. Should be one of mitlm or srilm, or provide -cmd instead. When using -cmd, -text TEXT -lm LM.gz -order N will be added to your command, so make sure that works."
fi

if [[ ! $LMOUT =~ '.rclm$' ]]; then
   error_exit "Invalid LM name $LMOUT: must end in .rclm"
fi
if [[ -d $LMOUT ]]; then
   warn "$LMOUT exists; will overwrite existing Rest-cost LM"
else
   run_cmd mkdir $LMOUT || error_exit "Can't create output directory; aborting."
fi

WORKDIR=`run_cmd mktemp -d $LMOUT.tmp.XXX` || error_exit "Can't create temp workdir $LMOUT.tmp.XXX"


# Train all the LMs, from 1-gram to ORDER-gram ones.
for O in $(seq 1 $ORDER); do
   RAW_LM=$WORKDIR/raw-${O}g.lm.gz
   LM=$WORKDIR/${O}g.lm.gz
   if [[ $TOOLKIT == mitlm ]]; then
      run_cmd "zcat $TEXT | perl -ple '"'s/^\s*(.*?)\s*$/$1/; s/\s+/ /g;'"' | time-mem $MITLM_CMD -order $O -text /dev/stdin -write-lm $LM"
   else
      run_cmd "time-mem $TRAIN_CMD -order $O -text $TEXT -lm $RAW_LM" &&
      run_cmd "lm_sort.pl $RAW_LM $LM"
   fi || error_exit "Error building $LM"
done

FULL_LM=$WORKDIR/${ORDER}g.lm.gz

# Extract the n-grams found in the full LM for each order.
for O in `seq 1 $ORDER`; do
   BEGIN="$O-grams:"
   if (( O < $ORDER )); then
      END=$((O+1))"-grams:"
   else
      END="end"
   fi
   #echo $FULL_LM $O-grams BEGIN=$BEGIN END=$END
   verbose 1 "[`date`]" Extracting $O-grams from $FULL_LM into $WORKDIR/$O-grams
   zcat $FULL_LM |
      sed -n "/^.$BEGIN/,/^.$END/p" |
      grep -v '^\\' | grep -v '^ *$' |
      sed -e 's/^[^	]*	//' -e 's/	.*//' |
      li-sort.sh > $WORKDIR/$O-grams
done

for O in $(seq $((ORDER-1)) -1 1); do
   # Get all suffixes from n-grams in the full LM
   OUT=$WORKDIR/$O-gram-suffixes
   verbose 1 "[`date`]" Extracting $O-gram suffixes of higher order n-gram into $OUT
   {
      cat $WORKDIR/$((O+1))-grams
      if (( O+1 < ORDER )); then cat $WORKDIR/$((O+1))-gram-suffixes; fi
   } |
      sed -e 's/^[^ ]* //' |
      LC_ALL=C sort -u > $OUT

   # Score those suffixes using the lower-order LMs
   LOWER_LM=$WORKDIR/${O}g.lm.gz
   run_cmd "time-mem lm_eval -1qpl $LOWER_LM $WORKDIR/$O-gram-suffixes > $WORKDIR/$O-gram-lowerprob" ||
      error_exit "Error querying lower-order LM $LOWER_LM for rest costs. Aborting."

   # The lower-score LM will need explicit 0 values for back-off weights of
   # prefixes of the n-grams it contains.
   #  - list those prefixes
   OUT=$WORKDIR/$O-gram-suffix-prefixes
   verbose 1 "[`date`]" Putting explicit 0 back-off weights on $O-gram prefixes of suffixes of higher order n-grams
   {
      if (( O+1 < ORDER )); then cat $WORKDIR/$((O+1))-gram-suffixes; fi
      if (( O+2 < ORDER )); then cat $WORKDIR/$((O+1))-gram-suffix-prefixes; fi
   } |
      sed -e 's/ [^ ]*$//' |
      LC_ALL=C sort -u > $OUT

   #  - and put an explicit back-off weight of 0 on each of them.
   perl -le '
      open PREFIXES, $ARGV[0] or die "Cannot open $ARGV[0]: $!";
      my %prefixes = map { chomp; ($_, 1) } <PREFIXES>;
      print STDERR "prefix count: " . scalar keys %prefixes;
      open NGRAMS, $ARGV[1] or die "Cannot open $ARGV[1]: $!";
      while (<NGRAMS>) {
         chomp;
         my ($prob, $ngram) = split /\t/, $_;
         my $bo = exists $prefixes{$ngram} ? "\t0" : "";
         print "$_$bo";
      }
   ' $WORKDIR/$O-gram-suffix-prefixes $WORKDIR/$O-gram-lowerprob > $WORKDIR/$O-gram-lowerprob-bo ||
      error_exit "Error inserting explicit 0 back-off weights in $WORKDIR/$O-gram-lowerprob-bo. Aborting."
done

REST_COSTS_TMP=$WORKDIR/rest-costs.lm.gz
verbose 1 "[`date`]" Assembling the rest-cost component of the LM into $REST_COSTS_TMP
{
   echo ""
   echo '\data\'
   for n in $(seq 1 $((ORDER-1))); do
      LINES=`wc -l < $WORKDIR/$n-gram-lowerprob-bo`
      echo ngram $n=$LINES
   done
   echo ""

   for n in $(seq 1 $((ORDER-1))); do
      echo "\\$n-grams:"
      cat $WORKDIR/$n-gram-lowerprob-bo
      echo ""
   done

   echo "\\end\\"
} | gzip > $REST_COSTS_TMP

REST_COSTS=rest-costs.lm.gz
run_cmd cp $REST_COSTS_TMP $LMOUT/$REST_COSTS

FINAL_LM=${ORDER}g.lm.gz
run_cmd cp $FULL_LM $LMOUT/$FINAL_LM

pushd $LMOUT
   run_cmd arpalm2tplm.sh $REST_COSTS
   run_cmd arpalm2tplm.sh $FINAL_LM
popd

CONFIG=$LMOUT/config
{
   echo "Rest-cost LM v1.0 (NRC)"
   echo "full-lm=${ORDER}g.lm.tplm"
   echo "rest-costs=rest-costs.lm.tplm"
} > $CONFIG

verbose 0 "[`date`] SUCCESS: Rest-cost LM $LMOUT built."
