#!/bin/bash

# @file gen-jpt-parallel.sh 
# @brief generate a joint probability table in parallel.
#
# @author George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

echo 'gen-jpt-parallel.sh, NRC-CNRC, (c) 2007 - 2008, Her Majesty in Right of Canada' >&2

usage() {
   for msg in "$@"; do
      echo -- $msg >&2
   done
   cat <<==EOF== >&2

Usage: gen-jpt-parallel.sh [-n N][-rp RUNPARALLELOPTS][-o outfile]
          GPT GPTARGS file1_lang1 file1_lang2

Generate a joint phrase table for a single (large) file pair in parallel, by
splitting the pair into pieces, generating a joint table for each piece, then
concatenating the results, which are written to stdout (uncompressed).

Options:

  -n        Number of chunks in which to split the file pair. [4]
            Warning: if GPTARGS contains the -w option, the output will depend
            on the setting of this option: higher values of N will yield 
            slightly larger phrase tables!
  -rp       Provide custom run-parallel.sh options (enclose in quotes!).
  -o        Send output to outfile (compressed).
  GPT       Keyword to signal beginning of gen_phrase_tables options and args.
  GPTARGS   Arguments and options for gen_phrase_tables: these must include
            the two IBM model arguments, but should NOT include any output
            selection options (this is not checked), nor the names of text
            files (input is limited to file1_lang1 and file1_lang2). Any
            quotes in GPTARGS must be escaped.
            Warning: -w and -prune1 are applied to each chunk individually, so
            their semantics are affected by N.  Provide -prune1 to
            joint2cond_phrase_tables instead to preserve its normal semantics;
            -w cannot be made to act as if given to gen_phrase_tables.

==EOF==

   exit 1
}

error_exit() {
   for msg in "$@"; do
      echo $msg >&2
   done
   echo "Use -h for help." >&2
   exit 1
}

arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}

is_int() {
   if [ "`expr $1 + 0 2> /dev/null`" != "$1" ]; then
      error_exit "Invalid argument ($1) to $2 option - must be int."
   fi
}

NUM_JOBS=4
OUTFILE="-"
RP_OPTS=
VERBOSE=

while [ $# -gt 0 ]; do
   case "$1" in
   -n)         arg_check 1 $# $1; is_int $2 $1; NUM_JOBS=$2; shift;;
   -rp)        arg_check 1 $# $1; RP_OPTS="$RP_OPTS $2"; shift;;
   -o)         arg_check 1 $# $1; OUTFILE=$2; shift;;
   -v)         VERBOSE="-v";;
   -d|-debug)  DEBUG=1;;
   -notreally) NOTREALLY=1; DEBUG=1;;
   -h|-help)   usage;;
   --)         shift; break;;
   -*)         error_exit "Unknown option $1.";;
   GPT)        shift; break;;
   *)          error_exit "Unknown arg $1.";;
   esac
   shift
done

if [ $# -lt 4 ]; then
   error_exit "Missing IBM models and file pair arguments."
fi

GPTARGS=($@)
if [[ "$GPTARGS" =~ -v ]]; then
   VERBOSE="-v"
fi

file1=${GPTARGS[$#-2]}
file2=${GPTARGS[$#-1]}
unset GPTARGS[$#-2]
unset GPTARGS[$#-1]

WORKDIR=JPTPAR$$
test -d $WORKDIR || mkdir $WORKDIR

if [ ! -e $file1 ]; then
   error_exit "Input file $file1 doesn't exist"
fi
if [ ! -e $file2 ]; then
   error_exit "Input file $file2 doesn't exist"
fi

NL=`zcat -f $file1 | wc -l`
SPLITLINES=$(($NL/$NUM_JOBS))
if [ -n $(($NL%$NUM_JOBS)) ]; then
   SPLITLINES=$(($SPLITLINES+1))
fi

zcat -f $file1 | split -l $SPLITLINES - $WORKDIR/L1.
zcat -f $file2 | split -l $SPLITLINES - $WORKDIR/L2.

if [ $DEBUG ]; then
    echo "NUM_JOBS = $NUM_JOBS" >&2
    echo "RP_OPTS = <$RP_OPTS>" >&2
    echo "GPTARGS = <${GPTARGS[@]}>" >&2
    echo $file1 >&2
    echo $file2 >&2
    echo $WORKDIR >&2
    echo $NL >&2
    echo $SPLITLINES >&2
    echo Verbose: $VERBOSE >&2
fi

# if [ ! $DEBUG ]; then
#    trap 'rm -rf ${WORKDIR}.*' 0 2 3 13 14 15
# fi

CMDS_FILE=$WORKDIR/cmds
test -f $CMDS_FILE && \rm -f $CMDS_FILE

for src in `ls $WORKDIR/L1.*`; do
   tgt=${src/\/L1./\/L2.}
   suff=${src##*/L1.}
   echo "test ! -f $src || ((set -o pipefail; gen_phrase_tables -j ${GPTARGS[@]} $src $tgt | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/$suff.jpt.gz) && mv $src $src.done)" >> $CMDS_FILE
done

test $DEBUG && cat $CMDS_FILE

if [ -n $VERBOSE ]; then
    echo "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $NUM_JOBS" >&2
fi

test $NOTREALLY ||
   eval "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $NUM_JOBS"
RC=$?
if (( $RC != 0 )); then
   echo "problem calculating the joint frequencies (status=$RC) - quitting!" >&2
   exit $RC
fi


# Merging parts
test $DEBUG && echo merge_counts $OUTFILE $WORKDIR/*.jpt.gz

test $NOTREALLY ||
   eval "merge_counts $OUTFILE $WORKDIR/*.jpt.gz"
RC=$?
if (( $RC != 0 )); then
   echo "problems merging the joint frequencies (status=$RC) - quitting!" >&2
   exit $RC
fi

rm -rf ${WORKDIR}
