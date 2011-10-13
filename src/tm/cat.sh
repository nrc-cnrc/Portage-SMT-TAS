#!/bin/bash
# $Id$

# @file cat.sh 
# @brief Cool Alignment Training - run train_ibm in parallel mode.
#
# @author Eric Joanis
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

print_nrc_copyright cat.sh 2007
export PORTAGE_INTERNAL_CALL=1

usage() {
   for msg in "$@"; do
      echo -- $msg >&2
   done
   cat <<==EOF== >&2

Usage: cat.sh [-n N] [-pn PN] [-1n 1N] [-1pn 1PN] [-h(elp)]
       [-rp RUNPARALLELOPTS] <train_ibm options>

  Run train_ibm in parallel mode, splitting inputs in N chunks, running them on
  PN CPUs in parallel.

Options to control parallel processing:

  -n        Number of chunks in which to split the parallel corpus. [4]
  -pn       Parallelism level.  If reading and writing the models doesn't
            take too long, N = PN*2 might help get all the workers to finish at
            about the same time. [N].  
  -1n       Number of chunks to use in the IBM1 training phase. [N]
  -1pn      Parallelism level for the IBM1 training phase.
            [1N if -1n specified, else PN if -pn specified, else N]
  -h(elp)   Print this help message.
  -rp       Provide custom run-parallel.sh options.
  -v        Turn on verbose output of train_ibm.
  -vv       Turn on verbose output of cat.sh itself.
  -vvv      Turn on verbose output of run-parallel.sh and cat.sh.
  -d(ebug)  Add some debugging output and don't cleanup temp files.
  -notreally Just print the commands to execute, don't run them.

Model training options are documented in "train_ibm -h".

==EOF==

   exit 1
}

run_cmd() {
   if [[ "$1" = "-no-error" ]]; then
      shift
      RUN_CMD_NO_ERROR=1
   else
      RUN_CMD_NO_ERROR=
   fi
   date >&2
   echo "$*" >&2
   if [[ ! $NOTREALLY ]]; then
      MON_FILE=`mktemp ${TMPPFX}mon.run_cmd.XXXXXXXX`
      process-memory-usage.pl -s 1 30 $$ > $MON_FILE &
      MON_PID=$!
      eval time $*
      rc=$?
      kill -USR1 $MON_PID
      if (( `wc -l < $MON_FILE` > 1 )); then
         MON_VSZ=`egrep -ho 'vsz: [0-9.]+G' $MON_FILE 2> /dev/null | egrep -o "[0-9.]+" | sum.pl -m`
         MON_RSS=`egrep -ho 'rss: [0-9.]+G' $MON_FILE 2> /dev/null | egrep -o "[0-9.]+" | sum.pl -m`
         echo "run_cmd finished $*" >&2
         echo "run_cmd rc=$rc Max VMEM ${MON_VSZ}G Max RAM ${MON_RSS}G" >&2
      fi
      if [[ -z "$RUN_CMD_NO_ERROR" && "$rc" != 0 ]]; then
         echo "Exit status: $rc is not zero - aborting." >&2
         exit 1
      fi
   fi
}

SAVE_ARGS="$@"
NUM_JOBS=4
NUM_ITERS1=5
NUM_ITERS2=5
COUNT_OPTIONS=""
EST_OPTIONS=""
QUIET=""
declare -a TRAIN_IBM_OPTS
declare -a TRAIN_HMM_OPTS
while [[ $# -gt 0 ]]; do
   case "$1" in
   -n)         arg_check 1 $# $1; arg_check_int $2 $1; NUM_JOBS=$2; shift;;
   -pn)        arg_check 1 $# $1; arg_check_int $2 $1; NUM_CPUS=$2; shift;;
   -1n)        arg_check 1 $# $1; arg_check_int $2 $1; NUM_IBM1_JOBS=$2; shift;;
   -1pn)       arg_check 1 $# $1; arg_check_int $2 $1; NUM_IBM1_CPUS=$2; shift;;
   -rp)        arg_check 1 $# $1; RP_OPTS="$RP_OPTS $2"; shift;;
   -i)         arg_check 1 $# $1; INIT_MODEL=$2; shift;;
   -s)         arg_check 1 $# $1; SAVE_IBM1=$2; shift;;
   -n1)        arg_check 1 $# $1; arg_check_int $2 $1; NUM_ITERS1=$2; shift;;
   -n2)        arg_check 1 $# $1; arg_check_int $2 $1; NUM_ITERS2=$2; shift;;
   -bin)       BIN_MODELS=$1;;
   -hmm)       DO_HMM=1;;
   -mod|-ibm1|-ibm2|-count-only|-est-only)
               error_exit "The $1 option is reserved for use by cat.sh.";;
   -tobin|-frombin)
               error_exit "Use train_ibm directly for model conversions.";;
   -v|-r|-m|-vr|-rv)
               TRAIN_IBM_OPTS=("${TRAIN_IBM_OPTS[@]}" "$1");;
   -p|-p2|-speed|-filter-singletons|-beg|-end|-slen|-tlen|-bksize|-max-len)
               arg_check 1 $# $1
               TRAIN_IBM_OPTS=("${TRAIN_IBM_OPTS[@]}" "$1" "$2")
               shift;;
   -symmetrized) arg_check 1 $# $1; DO_SYM=1; SYM_OPT="$1 $2"; shift;;
   -rev-i)     arg_check 1 $# $1; REV_INIT_MODEL=$2; shift;;
   -rev-s)     arg_check 1 $# $1; REV_SAVE_IBM1=$2; shift;;
   -rev-model) arg_check 1 $# $!; REV_MODEL=$2; shift;;
   -anchor|-liang|-end-dist) 
               TRAIN_HMM_OPTS=("${TRAIN_HMM_OPTS[@]}" "$1");;
   -p0|-up0|-alpha|-lambda|-max-jump|-mimic|-word-classes-l1|-word-classes-l2|-map-tau|-lex-prune-ratio)
               test $1 = -mimic && DO_HMM=1
               arg_check 1 $# $1
               TRAIN_HMM_OPTS=("${TRAIN_HMM_OPTS[@]}" "$1" "$2")
               shift;;
   -vv)        QUIET=""; VERBOSE=1;;
   -vvv)       QUIET="-v"; VERBOSE=2;;
   -d|-debug)  DEBUG=1;;
   -notreally) NOTREALLY=1;;
   -h|-help)   usage;;
   --)         shift; break;;
   -*)         error_exit "Unknown option: $1.";;
   *)          break;;
   esac
   shift
done
if [[ ! $NUM_IBM1_CPUS ]]; then
   if [[ $NUM_IBM1_JOBS ]]; then
      NUM_IBM1_CPUS=$NUM_IBM1_JOBS
   elif [[ $NUM_CPUS ]]; then
      NUM_IBM1_CPUS=$NUM_CPUS
   else
      NUM_IBM1_CPUS=$NUM_JOBS
   fi
fi
if [[ ! $NUM_CPUS ]]; then
   NUM_CPUS=$NUM_JOBS;
fi
if [[ ! $NUM_IBM1_JOBS ]]; then
   NUM_IBM1_JOBS=$NUM_JOBS
fi

if [[ $# -lt 1 ]]; then
   error_exit "Missing output model name."
fi
if [[ $# -lt 3 ]]; then
   error_exit "No parallel corpus to process - nothing to do."
fi

MODEL=$1
shift

CORPUS=$*

TIMEFORMAT="Single-job-total: Real %3lR User %3lU Sys %3lS PCPU %P%%"
START_TIME=`date +"%s"`
trap '
   RC=$?
   echo "Master-Wall-Time $((`date +%s` - $START_TIME))s" >&2
   exit $RC
' 0

if [[ $DO_SYM ]]; then
   if [[ $MODEL =~ ".*[^a-zA-Z0-9]([a-zA-Z0-9]*)_given_([a-zA-Z0-9]*)" ]]; then
      L2_GIVEN_L1_NO_DOT="${BASH_REMATCH[1]}_given_${BASH_REMATCH[2]}"
      L1_GIVEN_L2_NO_DOT="${BASH_REMATCH[2]}_given_${BASH_REMATCH[1]}"
      L2_GIVEN_L1=".$L2_GIVEN_L1_NO_DOT"
   elif [[ -z "$REV_MODEL" ]]; then
      error_exit "Can't figure out L2 and L1 from $MODEL; use -rev-model or standard names containing L2_given_L1"
   else
      L2_GIVEN_L1_NO_DOT="l2_given_l1"
      L2_GIVEN_L1=".$L2_GIVEN_L1_NO_DOT"
   fi
   if [[ -z $REV_INIT_MODEL && -n $INIT_MODEL && ! $INIT_MODEL =~ "$L2_GIVEN_L1_NO_DOT" ]]; then
      error_exit "-i $INIT_MODEL not consistent with $MODEL; use -rev-i"
   fi
   if [[ -z $REV_SAVE_IBM1 && -n $SAVE_IBM1 && ! $SAVE_IBM1 =~ "$L2_GIVEN_L1_NO_DOT" ]]; then
      error_exit "-s $SAVE_IBM1 not consistent with $MODEL; use -rev-s"
   fi
else
   L2_GIVEN_L1=""
fi

if [[ $DEBUG ]]; then
   echo -n TRAIN_IBM_OPTS
   for x in "${TRAIN_IBM_OPTS[@]}"; do
      echo -n :"$x";
   done
   if [[ -n "$TRAIN_HMM_OPTS" ]]; then
      echo -n TRAIN_HMM_OPTS
      for x in "${TRAIN_HMM_OPTS[@]}"; do
         echo -n :"$x";
      done
   fi
   echo
   echo N=$NUM_JOBS PN=$NUM_CPUS 1N=$NUM_IBM1_JOBS 1PN=$NUM_IBM1_CPUS
   echo $SYM_OPT $L2_GIVEN_L1
fi >&2

for outputfile in $MODEL $REV_MODEL $SAVE_IBM1 $REV_SAVE_IBM1; do
   if [[ -n $outputfile && -e $outputfile ]]; then
      error_exit "File $outputfile already exists - won't overwrite"
   fi
done

if [[ $PBS_JOBID ]]; then
   if [[ $PBS_JOBID =~ "^[[:digit:]]+" ]]; then
      TMPPFX=$MODEL.tmp"${BASH_REMATCH[0]}"
   else
      TMPPFX=$MODEL.tmp$PBS_JOBID
   fi
else
   TMPPFX=$MODEL.tmp$$
fi
while [[ -d $TMPPFX ]]; do
   TMPPFX=$TMPPFX.1
done
TMPPFX=$TMPPFX/

mkdir $TMPPFX || error_exit "Can't create directory $TMPPFX - aborting."

if [[ $NUM_ITERS2 -lt 1 && -n "$SAVE_IBM1" && "$SAVE_IBM1" != "$MODEL" ]]; then
   echo "-n2 0 used, -s option ignored, saving to $MODEL" >&2
   SAVE_IBM1=$MODEL
   test -n "$REV_MODEL" && REV_SAVE_IBM1=$REV_MODEL
fi

if [[ $NUM_ITERS2 -lt 1 && -z "$SAVE_IBM1" ]]; then
   SAVE_IBM1=$MODEL
   test -n "$REV_MODEL" && REV_SAVE_IBM1=$REV_MODEL
fi

ITER=0

# Initialization of the model
CUR_REV_MODEL=""
if [[ -z "$INIT_MODEL" ]]; then
   if [[ $NUM_IBM1_JOBS -le 1 || $NUM_ITERS1 -lt 1 ]]; then
      if [[ $SAVE_IBM1 ]]; then
         CUR_MODEL=$SAVE_IBM1
         INIT_OUT="$BIN_MODELS $CUR_MODEL"
         if [[ $REV_SAVE_IBM1 ]]; then
            INIT_OUT="-rev-model $REV_SAVE_IBM1 $INIT_OUT"
            CUR_REV_MODEL=$REV_SAVE_IBM1
         fi
      else
         CUR_MODEL=${TMPPFX}ibm1$L2_GIVEN_L1.gz
         INIT_OUT="-bin $CUR_MODEL"
      fi
      run_cmd train_ibm "${TRAIN_IBM_OPTS[@]}" -n1 $NUM_ITERS1 -n2 0 \
                        $SYM_OPT $INIT_OUT $CORPUS
      ITER=$NUM_ITERS1
      NUM_ITERS1=0
   else
      CUR_MODEL=${TMPPFX}iter0.ibm1$L2_GIVEN_L1.gz
      run_cmd train_ibm "${TRAIN_IBM_OPTS[@]}" -n1 0 -n2 0 \
                        $SYM_OPT -bin $CUR_MODEL $CORPUS
   fi
else
   CUR_MODEL=$INIT_MODEL
   if [[ $REV_INIT_MODEL ]]; then
      CUR_REV_MODEL=$REV_INIT_MODEL
   fi
fi

# IBM1 training
while [[ $NUM_ITERS1 -gt 0 ]]; do
   CUR_REV_MODEL_OUT=""
   if [[ $NUM_IBM1_JOBS -le 1 ]]; then
      if [[ $SAVE_IBM1 ]]; then
         CUR_MODEL_OUT=$SAVE_IBM1
         IBM1_OUT="$BIN_MODELS $CUR_MODEL_OUT"
         if [[ $REV_SAVE_IBM1 ]]; then
            CUR_REV_MODEL_OUT=$REV_SAVE_IBM1
            IBM1_OUT="-rev-model $REV_SAVE_IBM1 $IBM1_OUT"
         fi
      else
         CUR_MODEL_OUT=${TMPPFX}ibm1$L2_GIVEN_L1.gz
         IBM1_OUT="-bin $CUR_MODEL_OUT"
      fi
      CUR_INIT_MODEL="-i $CUR_MODEL"
      if [[ $CUR_REV_MODEL ]]; then
         CUR_INIT_MODEL="-rev-i $CUR_REV_MODEL $CUR_INIT_MODEL"
      fi
      run_cmd train_ibm "${TRAIN_IBM_OPTS[@]}" -n1 $NUM_ITERS1 -n2 0 \
                        $SYM_OPT $CUR_INIT_MODEL $IBM1_OUT $CORPUS
      CUR_MODEL=$CUR_MODEL_OUT
      CUR_REV_MODEL=$CUR_REV_MODEL_OUT
      ITER=$NUM_ITERS1
      NUM_ITERS1=0
   else
      ITER=$((ITER + 1))
      echo >&2
      echo Starting iteration $ITER on `date` >&2
      CUR_INIT_MODEL="-i $CUR_MODEL"
      if [[ $CUR_REV_MODEL ]]; then
         CUR_INIT_MODEL="-rev-i $CUR_REV_MODEL $CUR_INIT_MODEL"
      fi
      JOBFILE=${TMPPFX}iter$ITER.jobs
      cat /dev/null > $JOBFILE
      COUNT_FILES=
      for JOB in `seq 1 $NUM_IBM1_JOBS`; do
         COUNT_FILE=${TMPPFX}iter$ITER.counts$JOB$L2_GIVEN_L1.gz
         COUNT_FILES="$COUNT_FILES $COUNT_FILE"
         echo "train_ibm ${TRAIN_IBM_OPTS[@]} -ibm1 -count-only" \
              "-mod $JOB:$NUM_IBM1_JOBS" \
              "$SYM_OPT $CUR_INIT_MODEL $COUNT_FILE $CORPUS" \
              "&> ${TMPPFX}iter$ITER.counts$JOB.log " >> $JOBFILE
      done
      run_cmd run-parallel.sh $RP_OPTS $QUIET $JOBFILE $NUM_IBM1_CPUS
      if [[ $NOTREALLY ]]; then
         cat $JOBFILE
      fi
      if [[ "$VERBOSE" -ge 1 && -z "$NOTREALLY" ]]; then
         head -10000 ${TMPPFX}iter$ITER.counts*.log >&2
      fi
      if [[ $NUM_ITERS1 -eq 1 && -n "$SAVE_IBM1" ]]; then
         CUR_MODEL_OUT=$SAVE_IBM1
         IBM1_OUT="$BIN_MODELS $CUR_MODEL_OUT"
         if [[ $REV_SAVE_IBM1 ]]; then
            CUR_REV_MODEL_OUT=$REV_SAVE_IBM1
            IBM1_OUT="-rev-model $REV_SAVE_IBM1 $IBM1_OUT"
         fi
      else
         CUR_MODEL_OUT=${TMPPFX}iter$ITER.ibm1$L2_GIVEN_L1.gz
         IBM1_OUT="-bin $CUR_MODEL_OUT"
      fi
      run_cmd train_ibm "${TRAIN_IBM_OPTS[@]}" -est-only $CUR_INIT_MODEL \
                        $SYM_OPT $IBM1_OUT $COUNT_FILES
      CUR_MODEL=$CUR_MODEL_OUT
      CUR_REV_MODEL=$CUR_REV_MODEL_OUT
      NUM_ITERS1=$((NUM_ITERS1 - 1))
   fi
done

# IBM2 / HMM training
if [[ $DO_HMM ]]; then
   MODEL2="hmm"
else
   MODEL2="ibm2"
fi
while [[ $NUM_ITERS2 -gt 0 ]]; do
   CUR_REV_MODEL_OUT=""
   ITER=$((ITER + 1))
   echo >&2
   echo Starting iteration $ITER on `date` >&2
   CUR_INIT_MODEL="-i $CUR_MODEL"
   if [[ $CUR_REV_MODEL ]]; then
      CUR_INIT_MODEL="-rev-i $CUR_REV_MODEL $CUR_INIT_MODEL"
   fi
   JOBFILE=${TMPPFX}iter$ITER.jobs
   cat /dev/null > $JOBFILE
   COUNT_FILES=
   for JOB in `seq 1 $NUM_JOBS`; do
      COUNT_FILE=${TMPPFX}iter$ITER.counts$JOB$L2_GIVEN_L1.gz
      COUNT_FILES="$COUNT_FILES $COUNT_FILE"
      echo "train_ibm ${TRAIN_IBM_OPTS[@]} ${TRAIN_HMM_OPTS[@]}" \
           "-$MODEL2 -count-only -mod $JOB:$NUM_JOBS" \
           "$SYM_OPT $CUR_INIT_MODEL $COUNT_FILE $CORPUS" \
           "&> ${TMPPFX}iter$ITER.counts$JOB.log " >> $JOBFILE
   done
   run_cmd run-parallel.sh $RP_OPTS $QUIET $JOBFILE $NUM_CPUS
   if [[ $NOTREALLY ]]; then
      cat $JOBFILE
   fi
   if [[ "$VERBOSE" -ge 1 && -z "$NOTREALLY" ]]; then
      head -10000 ${TMPPFX}iter$ITER.counts*.log >&2
   fi
   if [[ $NUM_ITERS2 -eq 1 ]]; then
      CUR_MODEL_OUT=$MODEL
      MODEL2_OUT="$BIN_MODELS $CUR_MODEL_OUT"
      if [[ $REV_MODEL ]]; then
         CUR_REV_MODEL_OUT=$REV_MODEL
         MODEL2_OUT="-rev-model $REV_MODEL $MODEL2_OUT"
      fi
   else
      CUR_MODEL_OUT=${TMPPFX}iter$ITER.$MODEL2$L2_GIVEN_L1.gz
      MODEL2_OUT="-bin $CUR_MODEL_OUT"
   fi
   run_cmd train_ibm "${TRAIN_IBM_OPTS[@]}" "${TRAIN_HMM_OPTS[@]}" -est-only \
                     $SYM_OPT $CUR_INIT_MODEL -$MODEL2 $MODEL2_OUT $COUNT_FILES
   CUR_MODEL=$CUR_MODEL_OUT
   CUR_REV_MODEL=$CUR_REV_MODEL_OUT
   NUM_ITERS2=$((NUM_ITERS2 - 1))
done

echo >&2
echo Done on `date` >&2

# Things finished successfully, clean up.
if [[ ! $DEBUG ]]; then
   rm -rf $TMPPFX
fi

exit
