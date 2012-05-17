#!/bin/bash

# @file gen-jpt-parallel.sh
# @brief generate a joint probability table in parallel.
#
# @author George Foster & Samuel Larkin
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
       GPT GPTARGS file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2

Generate a joint phrase table for file pairs in parallel, by splitting the pair
into pieces, generating a joint table for each piece, then merging the results,
which are written to stdout (uncompressed).

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

which-test.sh stripe.py || error_exit "You are missing a key component (stripe.py)."


# Verify that the requested number of chunks doesn't exceed the maximum allowed
# simultaneously opened files since merge_multi_column_counts needs to have
# them all opened at once.
MAX_ALLOWED_OPEN_FILE=$((`ulimit -n`-20))
[[ $NUM_JOBS -gt $MAX_ALLOWED_OPEN_FILE ]]  && error_exit "$0 doesn't support merging more than $MAX_ALLOWED_OPEN_FILE chunks.  Lower the number of chunks before restarting."


[[ $WORKERS ]] || WORKERS=$NUM_JOBS

[[ $# -lt 4 ]] && error_exit "Missing IBM models and file pair arguments."

GPTARGS=("$@")
ALIGN_OPT=
for i in ${!GPTARGS[@]}; do
   if [[ ${GPTARGS[$i]} =~ -v ]]; then
      VERBOSE="-v"
   elif [[ ${GPTARGS[$i]} =~ -ext ]]; then
      EXTERNAL_ALIGNER=1;
   fi
done
[[ $DEBUG ]] && echo "ALIGN_OPT=$ALIGN_OPT" >&2

FILE1=()
FILE2=()
FILE3=()
# Ask gen_phrase_tables what are the corpora in the user provided command.
[[ $DEBUG ]] && echo "${#GPTARGS[@]}: <${GPTARGS[@]}>" >&2
[[ $NW ]] && USING_W="-w $NW"
FILE_ARGS=(`gen_phrase_tables -file-args $USING_W "${GPTARGS[@]}"`)
[[ $? == 0 ]] || error_exit "gen_phrase_tables doesn't seem to like the arguments you provided after the GPT keyword."

NUM_FILE_ARGS=${#FILE_ARGS[@]}
[[ $DEBUG ]] && echo "${#FILE_ARGS[@]}: <${FILE_ARGS[@]}>" >&2
if [[ $EXTERNAL_ALIGNER ]]; then
   [[ $FILTER ]] && error_exit "You cannot use -ext with a filter."

   [[ `expr ${#FILE_ARGS[@]} % 3` -eq 0 ]] || error_exit "You are missing a file in your triplet"
   # Split the single list of files into src / tgt.
   for i in ${!FILE_ARGS[@]}; do
      if [[ `expr $i % 3` -eq 0 ]]; then
         # PUSH
         FILE1=("${FILE1[@]}" ${FILE_ARGS[$i]})
      elif [[ `expr $i % 3` -eq 1 ]]; then
         # PUSH
         FILE2=("${FILE2[@]}" ${FILE_ARGS[$i]})
      elif [[ `expr $i % 3` -eq 2 ]]; then
         # PUSH
         FILE3=("${FILE3[@]}" ${FILE_ARGS[$i]})
      else
         error_exit "Something really wrong!";
      fi
   done
   [[ $DEBUG ]] && echo "FILE1: ${#FILE1[@]}: <${FILE1[@]}>" >&2
   [[ $DEBUG ]] && echo "FILE2: ${#FILE2[@]}: <${FILE2[@]}>" >&2
   [[ $DEBUG ]] && echo "FILE3: ${#FILE3[@]}: <${FILE3[@]}>" >&2
   # There must be exactly the same number of "src" & "tgt" corpora.
   [[ ${#FILE1[@]} == ${#FILE2[@]} ]] || error_exit "We failed to split the corpora's list."
   [[ ${#FILE1[@]} == ${#FILE3[@]} ]] || error_exit "We failed to split the corpora's list."
else
   [[ `expr ${#FILE_ARGS[@]} % 2` -eq 0 ]] || error_exit "You are missing a corpus in your pair of corpora"
   # Split the single list of files into src / tgt.
   while [[ ${#FILE_ARGS[@]} > 0 ]]; do
   if [[ `expr ${#FILE_ARGS[@]} % 2` -eq 0 ]]; then
      # PUSH
      FILE1=("${FILE1[@]}" ${FILE_ARGS[0]})
   else
      # PUSH
      FILE2=("${FILE2[@]}" ${FILE_ARGS[0]})
   fi
   # POP
   #FILE_ARGS=(${FILE_ARGS[@]:0:$((${#FILE_ARGS[@]}-1))})
   FILE_ARGS=(${FILE_ARGS[@]:1})
   done
   [[ $DEBUG ]] && echo "FILE1: ${#FILE1[@]}: <${FILE1[@]}>" >&2
   [[ $DEBUG ]] && echo "FILE2: ${#FILE2[@]}: <${FILE2[@]}>" >&2
   # There must be exactly the same number of "src" & "tgt" corpora.
   [[ ${#FILE1[@]} == ${#FILE2[@]} ]] || error_exit "We failed to split the corpora's list."
fi

# We will remove the corpora from the list of arguments since we will need to
# replace them by chunks that we will generate later.
[[ $DEBUG ]] && echo "BEFORE: ${#GPTARGS[@]}: <${GPTARGS[@]}>" >&2
GPTARGS=("${GPTARGS[@]:0:$((${#GPTARGS[@]}-${NUM_FILE_ARGS}))}")
[[ $DEBUG ]] && echo "AFTER: ${#GPTARGS[@]}: <${GPTARGS[@]}>" >&2

# Make sure all files are accessible
for file in ${FILE1[@]} ${FILE2[@]} ${FILE3[@]}; do
   [[ -e $file ]] || error_exit "Input file $file doesn't exist"
done

WORKDIR=JPTPAR$$
test -d $WORKDIR || mkdir $WORKDIR


# Start timing from this point.
TIMEFORMAT="Single-job-total: Real %3Rs User %3Us Sys %3Ss PCPU %P%%"
START_TIME=`date +"%s"`
trap '
   RC=$?
   echo "Master-Wall-Time $((`date +%s` - $START_TIME))s" >&2
   exit $RC
' 0


if [[ $DEBUG ]]; then
   echo "NUM_JOBS = $NUM_JOBS"
   echo "WORKERS = $WORKERS"
   echo "NW = $NW"
   echo "RP_OPTS = <$RP_OPTS>"
   echo "GPTARGS = <${GPTARGS[@]}>"
   echo source corpora: ${FILE1[@]}
   echo target corpora: ${FILE2[@]}
   echo alignment file: ${FILE3[@]}
   echo WORKDIR: $WORKDIR
   echo Verbose: $VERBOSE
fi >&2 # send everything to STDERR

# if [ ! $DEBUG ]; then
#    trap 'rm -rf ${WORKDIR}.*' 0 2 3 13 14 15
# fi

CMDS_FILE=$WORKDIR/cmds
test -f $CMDS_FILE && \rm -f $CMDS_FILE

[[ $VERBOSE ]] && echo "Creating voc files." >&2
if [[ ! $NOTREALLY ]]; then
   if [[ -n "$NW" ]]; then
   time {
      zcat -f ${FILE1[@]} | get_voc | sed 's/^|||$/___|||___/' > $WORKDIR/voc.1
      zcat -f ${FILE2[@]} | get_voc | sed 's/^|||$/___|||___/' > $WORKDIR/voc.2
   }
   fi
fi

declare -a ARG_STRING
for i in ${!GPTARGS[@]}; do
   if [[ ${GPTARGS[$i]} =~ " " ]]; then
      # Quote arguments with spaces.
      ARG_STRING="$ARG_STRING \"${GPTARGS[$i]//\"/\\\"}\""
   else
      ARG_STRING="$ARG_STRING ${GPTARGS[$i]}"
   fi
done

[[ $VERBOSE ]] && echo "Building command list." >&2
# Build the command list.
for idx in `seq -w 0 $(($NUM_JOBS-1))`; do
   src="zcat -f ${FILE1[@]} | stripe.py -i $idx -m $NUM_JOBS"
   tgt="zcat -f ${FILE2[@]} | stripe.py -i $idx -m $NUM_JOBS"
   ali="zcat -f ${FILE3[@]} | stripe.py -i $idx -m $NUM_JOBS"

   if [[ ! $NOTREALLY ]]; then
   if [[ -n "$NW" ]]; then
         WOPS="-w $NW -wf $WORKDIR/$idx.wp -wfvoc $WORKDIR/voc"
      time {
            eval $src | get_voc | sed 's/^|||$/___|||___/' > $WORKDIR/$idx.voc.1
            eval $tgt | get_voc | sed 's/^|||$/___|||___/' > $WORKDIR/$idx.voc.2
      }
   fi
   fi
   if [[ $EXTERNAL_ALIGNER ]]; then
      echo "test -f $WORKDIR/$idx.done || { { set -o pipefail; gen_phrase_tables $WOPS -j $ARG_STRING <($src) <($tgt) <($ali) | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/$idx.jpt.gz; } && touch $WORKDIR/$idx.done; }"
   else
      echo "test -f $WORKDIR/$idx.done || { { set -o pipefail; gen_phrase_tables $WOPS -j $ARG_STRING <($src $FILTER) <($tgt $FILTER) | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/$idx.jpt.gz; } && touch $WORKDIR/$idx.done; }"
   fi
done > $CMDS_FILE

[[ $DEBUG ]] && cat $CMDS_FILE >&2


[[ $VERBOSE ]] && echo "Creating merge command file." >&2
merge_commands="$WORKDIR/merge.cmds"
cat >> $merge_commands <<-cmd-head
	set -e
	set -o pipefail
cmd-head

if [[ -n "$NW" ]]; then
cat >> $merge_commands <<-End-of-message
	test -s "$WORKDIR/wp-cnts.1" || cat $WORKDIR/*.wp.1 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/wp-cnts.1
	test -s "$WORKDIR/wp-cnts.2" || cat $WORKDIR/*.wp.2 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/wp-cnts.2

	test -s "$WORKDIR/voc-cnts.1" || cat $WORKDIR/*.voc.1 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/voc-cnts.1
	test -s "$WORKDIR/voc-cnts.2" || cat $WORKDIR/*.voc.2 | LC_ALL=C $SORT_DIR sort | LC_ALL=C uniq -c > $WORKDIR/voc-cnts.2

	test -f "$WORKDIR/jpt.wp1.gz" || gen_jpt_filter_tool -l1 $WORKDIR/{voc-cnts,wp-cnts}.1 | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/jpt.wp1.gz
	test -f "$WORKDIR/jpt.wp2.gz" || gen_jpt_filter_tool -l2 $WORKDIR/{voc-cnts,wp-cnts}.2 | LC_ALL=C $SORT_DIR sort | gzip > $WORKDIR/jpt.wp2.gz
End-of-message
   WPFILES="$WORKDIR/jpt.wp1.gz $WORKDIR/jpt.wp2.gz"
fi

echo "[[ \"$OUTFILE\" != \"-\" ]] && [[ -f "$OUTFILE" ]] || merge_multi_column_counts $ALIGN_OPT $OUTFILE $WORKDIR/*.jpt.gz $WPFILES" >> $merge_commands

[[ $DEBUG ]] && cat $merge_commands >&2



[[ $VERBOSE ]] && echo "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $WORKERS" >&2

[[ $NOTREALLY ]] || eval "run-parallel.sh $VERBOSE $RP_OPTS $CMDS_FILE $WORKERS"
RC=$?
if (( $RC != 0 )); then
   echo "problem calculating the joint frequencies (status=$RC) - quitting!" >&2
   exit $RC
fi


# Merging parts
[[ $VERBOSE ]] && echo "Merging the final ouput(s)" >&2

if [[ ! $NOTREALLY ]]; then
   time {
      source $merge_commands
      RC=$?
   }
   if (( $RC != 0 )); then
      echo "problems merging the joint frequencies (status=$RC) - quitting!" >&2
      exit $RC
   fi
fi

# Clean up.
[[ $DEBUG ]] || rm -rf ${WORKDIR}

exit
