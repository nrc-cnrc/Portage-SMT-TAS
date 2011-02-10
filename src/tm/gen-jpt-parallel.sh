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

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

print_nrc_copyright gen-jpt-parallel.sh 2007
export PORTAGE_INTERNAL_CALL=1

usage() {
   for msg in "$@"; do
      echo -- $msg >&2
   done
   cat <<==EOF== >&2

Usage: gen-jpt-parallel.sh [-n N][-w NW][-rp RUNPARALLELOPTS][-o OUTFILE]
       GPT GPTARGS file1_lang1 file1_lang2

Generate a joint phrase table for a single (large) file pair in parallel, by
splitting the pair into pieces, generating a joint table for each piece, then
merging the results, which are written to stdout (uncompressed).

Options:

  -n N      Number of chunks in which to split the file pair. [4]
  -nw W     Number of workers to run in parallel. [N]
  -w NW     Add NW best IBM1 translations for src and tgt words that occur in
            given files but have no have phrase translations. This is similar
            to using -w for non-parallel phrase extraction from the given
            files, but uses a slightly different (probably better) algorithm;
            also, its results are independent of the number of parallel jobs N.
  -rp       Provide custom run-parallel.sh options (enclose in quotes!).
  -o        Send output to OUTFILE (compressed if it has a .gz extension).
  GPT       Keyword to signal beginning of gen_phrase_tables options and args.
  GPTARGS   Arguments and options for gen_phrase_tables: these must include
            the two IBM model arguments, but should NOT include any output
            selection options (this is not checked), nor the names of text
            files (input is limited to file1_lang1 and file1_lang2). Any
            quotes in GPTARGS must be escaped.
            Warning: -w and -prune1 are applied to each chunk individually, so
            their semantics are affected by N.  Provide -prune1 to
            joint2cond_phrase_tables instead to preserve its normal semantics;
            provide -w to gen-jpt-parallel.sh to get N-independent behaviour.

==EOF==

   exit 1
}

NUM_JOBS=4
NW=
OUTFILE="-"
RP_OPTS=
VERBOSE=
WORKERS=

while [ $# -gt 0 ]; do
   case "$1" in
   -n)         arg_check 1 $# $1; arg_check_int $2 $1; NUM_JOBS=$2; shift;;
   -nw)        arg_check 1 $# $1; arg_check_int $2 $1; WORKERS=$2; shift;;
   -w)         arg_check 1 $# $1; arg_check_int $2 $1; NW=$2; shift;;
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

[[ $WORKERS ]] || WORKERS=$NUM_JOBS

[[ $# -lt 4 ]] && error_exit "Missing IBM models and file pair arguments."

GPTARGS=($@)
if [[ "$GPTARGS" =~ -v ]]; then
   VERBOSE="-v"
fi

#echo gen_phrase_tables -num-file-args ${GPTARGS[@]} >&2
NUM_FILE_ARGS=`gen_phrase_tables -num-file-args ${GPTARGS[@]}`
if [[ $? != 0 ]]; then
   error_exit "gen_phrase_tables doesn't seem to like the arguments you provided after the GPT keyword."
elif [[ $NUM_FILE_ARGS != 2 ]]; then
   error_exit "Currently, gen-jpt-parallel.sh only works with exactly two corpus files; you provided $NUM_FILE_ARGS.";
fi

file1=${GPTARGS[$#-2]}
file2=${GPTARGS[$#-1]}
unset GPTARGS[$#-2]
unset GPTARGS[$#-1]

WORKDIR=JPTPAR$$
test -d $WORKDIR || mkdir $WORKDIR

[[ -e $file1 ]] || error_exit "Input file $file1 doesn't exist"
[[ -e $file2 ]] || error_exit "Input file $file2 doesn't exist"


# Start timing from this point.
TIMEFORMAT="Single-job-total: Real %3Rs User %3Us Sys %3Ss PCPU %P%%"
START_TIME=`date +"%s"`
trap '
   RC=$?
   echo "Master-Wall-Time $((`date +%s` - $START_TIME))s" >&2
   exit $RC
' 0


NL=`zcat -f $file1 | wc -l`
SPLITLINES=$(($NL/$NUM_JOBS))
if [ -n $(($NL%$NUM_JOBS)) ]; then
   SPLITLINES=$(($SPLITLINES+1))
fi

time {
   zcat -f $file1 | split -a 4 -l $SPLITLINES - $WORKDIR/L1.
   zcat -f $file2 | split -a 4 -l $SPLITLINES - $WORKDIR/L2.
}

if [[ $DEBUG ]]; then
   echo "NUM_JOBS = $NUM_JOBS"
   echo "WORKERS = $WORKERS"
   echo "NW = $NW"
   echo "RP_OPTS = <$RP_OPTS>"
   echo "GPTARGS = <${GPTARGS[@]}>"
   echo $file1
   echo $file2
   echo $WORKDIR
   echo $NL
   echo $SPLITLINES
   echo Verbose: $VERBOSE
fi >&2 # send everything to STDERR

# if [ ! $DEBUG ]; then
#    trap 'rm -rf ${WORKDIR}.*' 0 2 3 13 14 15
# fi

CMDS_FILE=$WORKDIR/cmds
test -f $CMDS_FILE && \rm -f $CMDS_FILE

if [[ -n "$NW" ]]; then
   time {
   get_voc $file1 > $WORKDIR/voc.1
   get_voc $file2 > $WORKDIR/voc.2
   }
fi

# Build the command list.
for src in `ls $WORKDIR/L1.*`; do
   tgt=${src/\/L1./\/L2.}
   suff=${src##*/L1.}
   if [[ -n "$NW" ]]; then
      WOPS="-w $NW -wf $WORKDIR/$suff.wp -wfvoc $WORKDIR/voc"
      time {
      get_voc $src > $WORKDIR/$suff.voc.1
      get_voc $tgt > $WORKDIR/$suff.voc.2
      }
   fi
   echo "test ! -f $src || ((set -o pipefail; gen_phrase_tables $WOPS -j ${GPTARGS[@]} $src $tgt | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/$suff.jpt.gz) && mv $src $src.done)" >> $CMDS_FILE
done

test $DEBUG && cat $CMDS_FILE


[[ -n $VERBOSE ]] && echo "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $WORKERS" >&2

[[ $NOTREALLY ]] || eval "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $WORKERS"
RC=$?
if (( $RC != 0 )); then
   echo "problem calculating the joint frequencies (status=$RC) - quitting!" >&2
   exit $RC
fi


# Merging parts
test $DEBUG && echo merge_counts $OUTFILE $WORKDIR/*.jpt.gz >&2

if [[ ! $NOTREALLY ]]; then

    if [[ -n "$NW" ]]; then

      time {
       cat $WORKDIR/*.wp.1 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/wp-cnts.1
       cat $WORKDIR/*.wp.2 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/wp-cnts.2

       cat $WORKDIR/*.voc.1 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/voc-cnts.1
       cat $WORKDIR/*.voc.2 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/voc-cnts.2

       gen_jpt_filter_tool -l1 $WORKDIR/{voc-cnts,wp-cnts}.1 | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/jpt.wp1.gz
       gen_jpt_filter_tool -l2 $WORKDIR/{voc-cnts,wp-cnts}.2 | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/jpt.wp2.gz
        
       WPFILES="$WORKDIR/jpt.wp1.gz $WORKDIR/jpt.wp2.gz"
       }
    fi

    time {
       eval "merge_counts $OUTFILE $WORKDIR/*.jpt.gz $WPFILES"
       RC=$?
    }
    if (( $RC != 0 )); then
        echo "problems merging the joint frequencies (status=$RC) - quitting!" >&2
        exit $RC
    fi
fi

rm -rf ${WORKDIR}

exit
