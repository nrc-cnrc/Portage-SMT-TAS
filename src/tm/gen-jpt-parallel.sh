#!/bin/bash

# gen-jpt-parallel.sh - generate a joint probability table in parallel
#
# PROGRAMMER: George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

echo 'gen-jpt-parallel.sh, NRC-CNRC, (c) 2007, Her Majesty in Right of Canada' >&2

usage() {
   for msg in "$@"; do
      echo -- $msg >&2
   done
   cat <<==EOF== >&2

Usage: gen-jpt-parallel.sh [-n N][-rp RUNPARALLELOPTS]
          GPT GPTARGS file1_lang1 file1_lang2

Generate a joint phrase table for a single (large) file pair in parallel, by
splitting the pair into pieces, generating a joint table for each piece, then
concatenating the results, which are written to stdout (uncompressed).

Options:

  -n        Number of chunks in which to split the file pair. [4]
  -rp       Provide custom run-parallel.sh options (enclose in quotes!).
  GPT       Keyword to signal beginning of gen_phrase_tables options and args.
  GPTARGS   Arguments and options for gen_phrase_tables: these must include
            the two IBM model arguments, but should NOT include any output
            selection options (this is not checked), nor the names of text
            files (input is limited to file1_lang1 and file1_lang2).

==EOF==

   exit 1
}

run_cmd() {
   if [ "$1" = "-no-error" ]; then
      shift
      RUN_CMD_NO_ERROR=1
   fi
   if [ $VERBOSE ]; then
      echo "$*" >&2
   fi
   if [ -z "$RUN_CMD_NO_ERROR" -a "$rc" != 0 ]; then
      echo "Exit status: $rc is not zero - aborting."
      exit 1
   fi
   RUN_CMD_NO_ERROR=
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
RP_OPTS=
VERBOSE=

while [ $# -gt 0 ]; do
   case "$1" in
   -n)         arg_check 1 $# $1; is_int $2 $1; NUM_JOBS=$2; shift;;
   -rp)        arg_check 1 $# $1; RP_OPTS="$RP_OPTS $2"; shift;;
   -v)         VERBOSE="-v";;
   -d|-debug)  DEBUG=1;;
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
file1=${GPTARGS[$#-2]}
file2=${GPTARGS[$#-1]}
unset GPTARGS[$#-2]
unset GPTARGS[$#-1]

TMP=JPTPAR$$

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

zcat -f $file1 | split -l $SPLITLINES - $TMP.L1.
zcat -f $file2 | split -l $SPLITLINES - $TMP.L2.

if [ $DEBUG ]; then
    echo "NUM_JOBS = $NUM_JOBS" >&2
    echo "RP_OPTS = <$RP_OPTS>" >&2
    echo "GPTARGS = <${GPTARGS[@]}>" >&2
    echo $file1 >&2
    echo $file2 >&2
    echo $TMP >&2
    echo $NL >&2
    echo $SPLITLINES >&2
fi

if [ ! $DEBUG ]; then
    trap 'rm -f ${TMP}.*' 0 2 3 13 14 15
fi

CMDS_FILE=$TMP.cmds
test -f $CMDS_FILE && \rm -f $CMDS_FILE

for src in `ls $TMP.L1.*`; do
   tgt=`echo $src | perl -pe 's/[.]L1[.]/.L2./o;'`
   suff=`echo $src | perl -pe 's/.*[.]L1[.]//o;'`
   echo "gen_phrase_tables -j ${GPTARGS[@]} $src $tgt | gzip > $TMP.$suff.jpt.gz " >> $CMDS_FILE
done

if [ -n $VERBOSE ]; then
    echo "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $NUM_JOBS" >&2
fi

eval "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $NUM_JOBS"
RC=$?
if (( $RC != 0 )); then
   echo "problems with run-parallel.sh (status=$RC) - quitting!" >&2
   exit $RC
fi

zcat $TMP.*.jpt.gz | joint2cond_phrase_tables -ij
