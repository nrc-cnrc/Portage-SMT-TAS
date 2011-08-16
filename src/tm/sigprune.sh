#!/bin/bash
# vim:nowrap
# $Id$

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
   cat <<==EOF== >&2

Usage: $0 [OPTIONS] -threshold TH JPT SRC TGT [SIG]

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
  -n N      Split the work in N jobs/chunks (at most N with -w) [3].
  -np M     Number of simultaneous workers used to process the N chunks [N].
  -w W      Specifies the minimum number of lines in each block.
  -psub <O> Passes additional options to run-parallel.sh -psub.
  -rp   <O> Passes additional options to run-parallel.sh.

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -keep)               KEEP_INTERMEDIATE=1;;
   -threshold)          arg_check 1 $# $1; THRESHOLD=$2; shift;;
   -sigopts)            arg_check 1 $# $1; SIG_OPTS=$2; shift;;

   -n)                  arg_check 1 $# $1; PARALLELIZE_OPTS="$PARALLELIZE_OPTS -n $2"; shift;;
   -np)                 arg_check 1 $# $1; PARALLELIZE_OPTS="$PARALLELIZE_OPTS -np $2"; shift;;
   -w)                  arg_check 1 $# $1; PARALLELIZE_OPTS="$PARALLELIZE_OPTS -w $2"; shift;;
   -psub)               arg_check 1 $# $1; PARALLELIZE_OPTS="$PARALLELIZE_OPTS -psub '$2'"; shift;;
   -rp)                 arg_check 1 $# $1; PARALLELIZE_OPTS="$PARALLELIZE_OPTS -rp '$2'"; shift;;

   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done


#test $# -eq 0   && error_exit "Missing threshold value."
#THRESHOLD=$1; shift
test $# -eq 0   && error_exit "Missing jpt."
JPT=$1; shift
test $# -eq 0   && error_exit "Missing source corpus."
SRC=$1; shift
test $# -eq 0   && error_exit "Missing target corpus."
TGT=$1; shift
SIG=$1; shift || SIG="-"
test $# -gt 0   && error_exit "Superfluous arguments $*"


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

# Define the intermediate count file.
INTERMEDIATE_FILE="cnts.`basename $JPT`"
if [[ -e "$INTERMEDIATE_FILE" ]]; then
   INTERMEDIATE_EXITS=1
   if [[ "$JPT" -nt "$INTERMEDIATE_FILE" ]]; then
      error_exit "\n\tYour co-occurence count file ($INTERMEDIATE_FILE) is older than your JPT ($JPT)." \
         "\tTake the proper action to fix this situation."
   fi
fi


# How should the output look like.
if [[ $SIG == "-" ]]; then
   OUTPUT="cat"
else
   OUTPUT="cat > $SIG"
   [[ $SIG =~ ".gz$" ]] && OUTPUT="gzip > $SIG"
fi


# Try to reuse an already created intermediate file.
{
   if [[ $INTERMEDIATE_EXITS ]]; then
      echo "Warning: Reusing the co-occurence count file: $INTERMEDIATE_FILE" >&2
      zcat -f $INTERMEDIATE_FILE;
   else
      SIG_OUT=""
      if [[ $KEEP_INTERMEDIATE ]]; then
         SIG_OPTS="$SIG_OPTS -keep-intermediate-files"
         if [[ $INTERMEDIATE_FILE =~ ".gz$" ]]; then
            SIG_OUT="| tee >(gzip > $INTERMEDIATE_FILE)"
         else
            SIG_OUT="| tee $INTERMEDIATE_FILE"
         fi
      fi

      echo "Creating the co-occurence counts." >&2
      [[ -s mmfusa.src.tpsa ]] || build-tp-suffix-array.sh $SRC mmfusa.src >&2
      [[ -s mmfusa.tgt.tpsa ]] || build-tp-suffix-array.sh $TGT mmfusa.tgt >&2

      CMD="time parallelize.pl -stripe $PARALLELIZE_OPTS \"phrasepair-contingency --sigfet mmfusa src tgt < $JPT\" $SIG_OUT"
      [[ $DEBUG ]] && echo "Contingency command: $CMD" >&2
      eval "$CMD" || error_exit "Problem with parallelize.pl!"
   fi;
} \
| sigprune_fet \
| eval $FILTER \
| eval $OUTPUT

if [[ ! $KEEP_INTERMEDIATE ]]; then
   # NOTE: in order to get a 0 exit status the following is writing to always
   # produce true unless there is really a problem removing the files.
   [[ ! -e mmfusa.src.tpsa ]] || rm -r mmfusa.src.tpsa
   [[ ! -e mmfusa.tgt.tpsa ]] || rm -r mmfusa.tgt.tpsa
fi

exit

