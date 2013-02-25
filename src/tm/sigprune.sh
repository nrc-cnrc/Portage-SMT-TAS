#!/bin/bash
# vim:nowrap

# @file sigprune.sh
# @brief Wraps Howard's significance pruning into one script.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

# WARNING:
# DESIGN NOTE:
#  Since creating the co-occurence count file takes a long time but applying
#  fisher's exact test is rather quick, the intermediate file will be defined
#  as the co-occurence counts instead of the output of fisher's exact test.
#  This will allows us to rerun significance pruning with a difference test
#  without having to rerun the co-occurence.



# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

print_nrc_copyright sigprune.sh 2011

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG [OPTIONS] -threshold TH JPT SRC TGT [SIG]

  Given a threshold TH, JPT, SRC & TGT, filter the JPT according to Howard's significance pruning.
  JPT a joint frequency phrase table [IN].
  SRC source corpus used to create JPT [IN].
  TGT target corpus used to create JPT [IN].
  SIG a joint frequency phrase table filtered according to Howard's significance pruning [OUT].

Options:

  -threshold THR  Keep all phrase pairs with a log(significance) <= -THR.
                  Larger values of THR result in smaller tables.
                  Note that THR must be a positive real number or one of
                  the following pre-defined constants:
                  'a+e' (read alpha + epsilon) is the significance threshold
                        such that <1,1,1> phrase pairs are filtered out.
                  'a-e' (read alpha - epsilon) is the threshold such that
                        <1,1,1> phrase pairs are kept.
                  Note: a (alpha) is the significance level of <1,1,1> phrase
                  pairs, as discussed in Johnson et al, EMNLP 2007.

  -keep     Keep the co-occurrence counts file.
  -np M     Number of simultaneous workers used to process the chunks [3].
  -n N      Split the JPT in N jobs/chunks [min(M,ceil(|JPT|/J))].
  -w w      Specifies the minimum number of tokens for each suffix arrays [1,000,000].
  -W W      Specifies the maximum number of tokens for each suffix arrays [150,000,000].
  -j J      Specifies the minimum of lines per JPT chunk [10,000].
            (Note: -j has no effect if -n is specified)
  -psub <O> Passes additional options to run-parallel.sh -psub.
  -rp   <O> Passes additional options to run-parallel.sh.

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

A note about parallelism:

  This script parallelizes its work in a matrix fashion: the parallel corpus
  is divided in chunks that make manageably sized suffix arrays (as controlled
  by -w and -W, whose defaults are adjusted to make sure each job never needs
  more than 4GB of RAM); the JPT is also divided in chunks to meet the desired
  parallelism level (as controlled by -j or -n).
  
  Typical usage: specify the desired parallelism with -np, and let the other
  parameters default.  You will get fewer workers when your job is small.

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
NP=3
w=1000000
W=150000000
J=10000
while [ $# -gt 0 ]; do
   case "$1" in
   -keep)               KEEP_INTERMEDIATE=1;;
   -threshold)          arg_check 1 $# $1; THRESHOLD=$2; shift;;

   -n)                  arg_check 1 $# $1; N=$2; shift;;
   -np)                 arg_check 1 $# $1; NP=$2; shift;;
   -w)                  arg_check 1 $# $1; w=$2; shift;;
   -W)                  arg_check 1 $# $1; W=$2; shift;;
   -j)                  arg_check 1 $# $1; J=$2; shift;;
   -psub)               arg_check 1 $# $1; PSUB_OPTS="$PSUB_OPTS $2"; shift;;
   -rp)                 arg_check 1 $# $1; RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $2"; shift;;

   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

# User specifies W has maximum number of tokens in either source or target
# shard but we will use W has maximum tokens in combining source shard and
# target shard.
W=$((2*$W))
w=$((2*$w))

# Validate the minimum shards size against the maximum shards size.
[[ $w -le $W ]] || error_exit "w must be smaller or equal to W."

debug "RUN_PARALLEL_OPTS: $RUN_PARALLEL_OPTS"

test $# -eq 0   && error_exit "Missing jpt."
JPT=$1; shift
test $# -eq 0   && error_exit "Missing source corpus."
SRC=$1; shift
test $# -eq 0   && error_exit "Missing target corpus."
TGT=$1; shift
SIG=$1; shift || SIG="-"
test $# -gt 0   && error_exit "Superfluous arguments $*"

[[ -r $JPT && -s $JPT ]] || error_exit "JPT file $JPT empty or non-existent or not readable"
[[ -r $SRC && -s $SRC ]] || error_exit "SRC file $SRC empty or non-existent or not readable"
[[ -r $TGT && -s $TGT ]] || error_exit "TGT file $TGT empty or non-existent or not readable"

if [[ $THRESHOLD == "a-e" ]]; then
   # Keeps <1,1,1>
   FILTER="perl -nle \"BEGIN{use encoding 'UTF-8';} @a = split(/\t/); print \\\$a[8] if (\\\$a[2] == 2 or \\\$a[2] == 3)\""
elif [[ $THRESHOLD == "a+e" ]]; then
   # Filter out <1,1,1>
   FILTER="perl -nle \"BEGIN{use encoding 'UTF-8';} @a = split(/\t/); print \\\$a[8] if (\\\$a[2] == 3)\""
else
   # Validate threshold to be a real number.
   result=$(echo "scale=2; $THRESHOLD + 0" | bc -q 2>/dev/null)
   stat=$?
   if [[ $stat != 0 || -z "$result" || $THRESHOLD != $result ]]; then
      error_exit "You must provide a Real number for the threshold."
   fi
   perl -e "exit 1 if ($THRESHOLD <= 0)" || error_exit "Your threshold should be a positive value not $THRESHOLD.";
   # Keeps only those entries that meets the threshold the user provided.
   FILTER="perl -nle \"BEGIN{use encoding 'UTF-8';} @a = split(/\t/); print \\\$a[8] if (\\\$a[1] <= -$THRESHOLD)\""
fi
debug "FILTER=$FILTER"

# Define the intermediate count file.
INTERMEDIATE_FILE="sig.cnts.`basename $JPT`"
# Always keep the intermediate file compressed.
INTERMEDIATE_FILE=${INTERMEDIATE_FILE%%.gz}.gz
INTERMEDIATE_SIG_COUNTS="cat"
# Is there a valid intermediate file?
if [[ -n "`find . -maxdepth 1 -size +21c -name $INTERMEDIATE_FILE`" ]]; then
   INTERMEDIATE_EXITS=1
   if [[ "$JPT" -nt "$INTERMEDIATE_FILE" ]]; then
      error_exit "\n\tYour co-occurence count file ($INTERMEDIATE_FILE) is older than your JPT ($JPT)." \
         "\tTake the proper action to fix this situation."
   fi
else
   # By default we don't keep the intermediate count file.
   if [[ $KEEP_INTERMEDIATE ]]; then
      if [[ $INTERMEDIATE_FILE =~ ".gz$" ]]; then
         INTERMEDIATE_SIG_COUNTS="tee >(gzip > $INTERMEDIATE_FILE)"
      else
         INTERMEDIATE_SIG_COUNTS="tee $INTERMEDIATE_FILE"
      fi
   fi
fi


# How should the output look like.
if [[ $SIG == "-" ]]; then
   OUTPUT="cat"
else
   OUTPUT="cat > $SIG"
   [[ $SIG =~ ".gz$" ]] && OUTPUT="gzip > $SIG"
fi



# the mmsufa files will be named after the source and target files
MMSUFA_SRC=mmsufa.`basename $SRC`
MMSUFA_TGT=mmsufa.`basename $TGT`

WORKDIR=sigprune.sh.$$
mkdir $WORKDIR || error_exit "Cannot create workdir $WORDIR"
MMSUFA_CMDS=cmd.mmsufa
CONTINGENCY_CMDS=cmd.contingency
MERGE_CMDS=cmd.merge

START_TIME=`date +"%s"`
set -o pipefail
verbose 1 "Starting on `date`"
TIMEFORMAT="Sub-job-total: Real %3Rs User %3Us Sys %3Ss PCPU %P%%"
# Try to reuse an already created intermediate file.
if [[ $INTERMEDIATE_EXITS ]]; then
   warn "Reusing the co-occurence count file: $INTERMEDIATE_FILE"
   zcat -f $INTERMEDIATE_FILE
else
   # Let's try to create shards of even sizes smaller than W but of at least size w for the input corpora.
   verbose 1 "Calculating the total number of tokens in $SRC & $TGT"
   time {
      NUMBER_CORPORA_TOKENS=`zcat -f $SRC $TGT | wc -w`;
   }
   verbose 1 "There are $NUMBER_CORPORA_TOKENS in the source & target corpora combined."
   NUM_SHARDS=$(($NUMBER_CORPORA_TOKENS / $W))
   [[ $NUM_SHARDS -gt 1 ]] && W=$(($NUMBER_CORPORA_TOKENS / $NUM_SHARDS + 1))
   [[ $W -lt $w ]] && W=$w

   verbose 1 "Sharding the input corpora $SRC & $TGT in shards of ~$W tokens."
   time {
      paste <(zcat -f $SRC) <(zcat -f $TGT) \
         | perl -nle "BEGIN{\$c=0; sub c { close(S); close(T); } sub o { open(S, \">$WORKDIR/\$c.s\"); open(T, \">$WORKDIR/\$c.t\"); \$c++; \$total=0}; }(\$s, \$t)=split(/\t/); o() unless(fileno S); print S \$s; print T \$t; \$total+=scalar(split(/[ \t]+/, \$s)) + scalar(split(/[ \t]+/, \$t)); c() if (\$total > $W); END{c();}" \
         || error_exit "Error splitting the input corpora.";
   }


   # Sharding the joint phrase table.
   verbose 1 "Calculating the total number of phrase pairs in $JPT"
   time {
      NUMBER_OF_JPT_ENTRIES=`zcat -f $JPT | wc -l`;
   }
   verbose 1 "There are $NUMBER_OF_JPT_ENTRIES phrase pairs in the joint phrase table."
   if [[ -z $N ]]; then
      N=$(ceiling_quotient $NUMBER_OF_JPT_ENTRIES $J)
      N=$(($NP < $N ? $NP : $N))
      if [[ $N -gt 1 ]]; then
         JPT_SHARD_SIZE=$(($NUMBER_OF_JPT_ENTRIES / $N))
         if [[ $JPT_SHARD_SIZE -lt $J ]]; then
            N=$(($N - 1))
            JPT_SHARD_SIZE=$(($NUMBER_OF_JPT_ENTRIES / $N))
         fi
      else
         JPT_SHARD_SIZE=$NUMBER_OF_JPT_ENTRIES
      fi
   else
      JPT_SHARD_SIZE=$(($NUMBER_OF_JPT_ENTRIES / $N + 1))
   fi
   mkdir -p $WORKDIR/jpt || error_exit "Cannot create directory for jpts."
   verbose 1 "Sharding the joint phrase table in $N shards of ~$JPT_SHARD_SIZE phrase pairs."
   time {
      zcat -f $JPT | split --suffix-length=5 --numeric-suffixes --lines=$JPT_SHARD_SIZE - $WORKDIR/jpt/;
   }


   verbose 1 "Preparing memory mapped suffix array's commands."
   pushd $WORKDIR &> /dev/null || error_exit "Can't change dir to $WORKDIR"
   for src in *.s; do
      tgt=${src%%.s}.t
      if [[ -s $src.tpsa/msa ]]; then
         warn "Reusing existing $src.tpsa."
      else
         echo "pushd $WORKDIR && build-tp-suffix-array.sh $src"
      fi
      if [[ -s $tgt.tpsa/msa ]]; then
         warn "Reusing existing $tgt.tpsa."
      else
         echo "pushd $WORKDIR && build-tp-suffix-array.sh $tgt"
      fi
   done > $MMSUFA_CMDS
   [[ $DEBUG ]] && cat $MMSUFA_CMDS >&2

   verbose 1 "Preparing co-occurence counts commands."
   MODULO=$NP
   for jpt_shard in jpt/*; do
      jpt_shard=`basename $jpt_shard`
      for mmsufa in *.s; do
         mmsufa=${mmsufa%%.s}
         prefix=$WORKDIR/$mmsufa
         out=$WORKDIR/jpt${jpt_shard}_mmsufa${mmsufa}.out.gz
         # NOTE: not using stripe.py in order to make it easier to merge the output.
         #echo "zcat $JPT | stripe.py -i $jpt_shard -m $MODULO | phrasepair-contingency --sigfet $prefix s.tpsa t.tpsa > $out"
         echo "cat $prefix.{s,t}.tpsa/* &> /dev/null && phrasepair-contingency --sigfet $prefix s.tpsa t.tpsa < $WORKDIR/jpt/$jpt_shard | cut -f1-5 | gzip > $out"
      done
   done > $CONTINGENCY_CMDS
   [[ $DEBUG ]] && cat $CONTINGENCY_CMDS >&2

   verbose 1 "Preparing merging commands."
   for jpt_shard in jpt/*; do
      jpt_file="$WORKDIR/$jpt_shard"
      jpt_shard=`basename $jpt_shard`
      echo "merge_sigprune_counts $jpt_file $WORKDIR/jpt${jpt_shard}_mmsufa*.out.gz"
   done > $MERGE_CMDS
   [[ $DEBUG ]] && cat $MERGE_CMDS >&2
   popd &> /dev/null

   verbose 0 "Creating memory mapped suffix arrays."
   run-parallel.sh $RUN_PARALLEL_OPTS -psub "$PSUB_OPTS" $WORKDIR/$MMSUFA_CMDS $NP || error_exit "Problem generating memory mapped suffix arrays!"

   verbose 0 "Calculating phrase pairs co-occurences for each shard."
   run-parallel.sh $RUN_PARALLEL_OPTS -psub "$PSUB_OPTS" $WORKDIR/$CONTINGENCY_CMDS $NP || error_exit "Problem calculating phrase pair contingency!"

   verbose 0 "Calculating phrase pairs co-occurences for the entire joint phrase table."
   time {
      . $WORKDIR/$MERGE_CMDS;
   }
fi \
| sigprune_line_numbering \
| eval $INTERMEDIATE_SIG_COUNTS \
| sigprune_fet \
| eval $FILTER \
| eval $OUTPUT

# NOTE: SP::PrefixCode::b64 creates a line number required for Howard's significance pruning scripts.
# Can be replaced with sigprune_line_numbering.


verbose 0 "Done"
verbose 0 "Master-Wall-Time $((`date +%s` - $START_TIME))s"

if [[ $KEEP_INTERMEDIATE ]]; then
   # Make sure there was no problem generating the intermediate count file.
   if [[ -z "`find . -maxdepth 1 -size +21c -name $INTERMEDIATE_FILE`" ]]; then
      error_exit "Error generating a valid intermediate file ($INTERMEDIATE_FILE)"
   fi
fi

# TODO: Should we keep the mmsufa?
# Actually, even on the .gc.ca it took about 15 mins per mmsufa x 6.
[[ $DEBUG ]] || rm -r $WORKDIR || error_exit "Problem cleaning up the working directory."

exit

