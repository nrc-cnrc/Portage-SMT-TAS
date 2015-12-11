#!/bin/bash
#
# @file canoe-parallel.sh
# @brief wrapper to run canoe on a parallel machine or on a cluster.
#
# @author Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada


# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

print_nrc_copyright canoe-parallel.sh 2005
export PORTAGE_INTERNAL_CALL=1

PATH="$PATH:/usr/local/bin"

TIMEFORMAT="Single-job-total: Real %3Rs User %3Us Sys %3Ss PCPU %P%%"
CANOE=canoe
MIN_DEFAULT_NUM=4
MAX_DEFAULT_NUM=10
HIGH_SENT_PER_BLOCK=100
LOW_SENT_PER_BLOCK=40
NBEST_PREFIX=      # Will be use to merge chunks in append mode
NBEST_COMPRESS=    # Will be use to merge chunks in append mode
LATTICE_PREFIX=      # Ditto, for lattices
LATTICE_COMPRESS=    # Ditto, for lattices


usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: canoe-parallel.sh [options] canoe [canoe options] < <input>

  Divide the sentences to translate among several canoe instances running in
  parallel; the final output is reassembled to be identical to what a single
  canoe would have output with the same options and input.

  NB: if you are generating nbest lists and associated files (eg alignments,
  feature values), and concatenating them with canoe's -append option, all
  relevant options, including -append, must be given on the command line to
  this program (after the canoe keyword), rather than in a canoe config file.

Options:

  -resume jid   Tries to rerun parts that failed aka parts that aren't the
                expected size.  You must reuse the same original command
                that failed plus -resume <failed_JID> which looks like
                canoe-parallel.XXX.

  -lb           Run in block-oriented load-balancing mode: split input into
                J [=2N] blocks, N roughly even "heavy" ones, and J-N lighter
                ones; run them in heavy to light order.

  -lb-by-sent   Run in sentence-oriented load-balancing mode: each canoe worker
                reads all input sentences, but defers to a demon to get one
                sentence ID to translate at a time.

  -no-lb        Disable all load balancing: negates -lb and -lb-by-sent.
                Split input into roughly equal-sized contiguous blocks.

                [-lb-by-sent, but -no-lb if -f or canoe -append is used.]

  -j(ob) J      Used with -lb to specify the number of jobs J >= N.  [2N]
                (Meaningless without -lb.)

  -cleanup      Cleanup the run-parallel* logs after completion [don't]

  -ref ref-file: reference file for canoe (must occur *before* "canoe")

  -noc(luster): background all jobs with & [default if not on a cluster]

  -n(um) N:     split the input into N blocks. [# input sentences / $HIGH_SENT_PER_BLOCK,
                but in [$MIN_DEFAULT_NUM,$MAX_DEFAULT_NUM] in cluster mode; otherwise number of CPUs]

  -pn PN:       the number of jobs to run simultaneously in cluster mode.
                Useful if you want more blocks than parallel workers.

  -h(elp):      print this help message

  -v(erbose):   increment the verbosity level by 1 (may be repeated)
                [default: verbosity level 1]
                Note that this is not the same as canoe's -v option, which
                should be provided *after* the "canoe" keyword if desired.

  -q(quiet):    suppress all verbose output, including verbose output by canoe

  -d(ebug):     turn the debug mode on.

  canoe <canoe options>: this mandatory argument must occur after all
                options to this script and before all regular canoe options.

Cluster mode options (ignored on non-clustered machines):

  -highmem:     use 2 cpus per job, for extra memory.

  -nolocal:     don't run any jobs locally; instead use psub for all of them
                [run one worker locally]

  -psub|psub-opts|psub-options <psub options>: specific options to pass to
                psub, such as specific resource requirements.  To specify
                multiple options, put them together in quotes, or repeat
                this option.

  -qsub|qsub-opts|qsub-options <qsub options>: specific options to pass to
                qsub (through psub's -qsparams options).  To specify
                multiple options, put them together in quotes, or repeat
                this option.

==EOF==

   exit 1
}


START_TIME=`date +"%s"`

trap '
   RC=$?
   echo "Master-Wall-Time $((`date +%s` - $START_TIME))s" >&2
   exit $RC
' 0



# Command line processing ; "declare -a" declares array variables
declare -a CANOEOPTS
RUN_PARALLEL_OPTS=
NUM=
NUMBER_OF_JOBS=1
PARNUM=
VERBOSE=1
DEBUG=
GOTCANOE=
SAVE_ARGS="$@"
REF=
LOAD_BALANCING=default
RESUME=
while [ $# -gt 0 ]; do
   case "$1" in
   -noc|-nocluster)RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -nocluster";;
   -resume)        arg_check 1 $# $1; RESUME=$2; shift;;
   -lb)            LOAD_BALANCING=byblock;;
   -lb-by-sent)    LOAD_BALANCING=bysent;;
   -no-lb)         LOAD_BALANCING=;;
   -ref)           arg_check 1 $# $1; REF=$2; shift;;
   -n|-num)        arg_check 1 $# $1; NUM=$2; shift;;
   -j|-job)        arg_check 1 $# $1; NUMBER_OF_JOBS=$2; shift;;
   -pn)            arg_check 1 $# $1; PARNUM=$2; shift;;
   -cleanup)       RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
   -highmem)       RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
   -nolocal)       RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
   -local)         arg_check 1 $# $1; RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -local $2"; shift;;
   -psub|-psub-opts|-psub-options)
                   arg_check 1 $# $1; PSUBOPTS="$PSUBOPTS $2"; shift;;
   -qsub|-qsub-opts|-qsub-options)
                   arg_check 1 $# $1; QSUBOPTS="$QSUBOPTS $2"; shift;;
   -v|-verbose)    VERBOSE=$(( $VERBOSE + 1 ))
                   RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
   -q|-quiet)      VERBOSE=0
                   RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
   -d|-debug)      DEBUG=1; RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -d";;
   -h|-help)       usage;;
   canoe)          shift; GOTCANOE=1; CANOEOPTS=("$@"); break;;
   *)              error_exit "Unknown option $1." \
                   "Possibly missing the 'canoe' argument before regular canoe options?"
                   ;;
   esac
   shift
done

# Disable load balancing if using append mode
APPEND=`echo $@ | egrep -oe '-append'`
debug "APPEND: $APPEND"
# Note: we used to disable -lb when -append was present, but now the two
# options are supported together - this script does the -append function at the
# end; not efficient for very large amounts of data, though.

if [[ $LOAD_BALANCING == default ]]; then
   if [[ $APPEND ]]; then
      # append mode is more efficient without any load balancing, and is
      # usually used with very large input anyway, which is also better handled
      # without load balancing.
      LOAD_BALANCING=
   else
      # I'm guessing this is going to be most efficient by default.
      LOAD_BALANCING=bysent
   fi
fi

debug LOAD_BALANCING=$LOAD_BALANCING

if [[ $VERBOSE == 1 && $LOAD_BALANCING == bysent ]]; then
   RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -quiet-daemon"
fi

# EJJ This breaks the resume mode, but is necessary on the GPSC to avoid file
# name clashes between different jobs running in similarly-initialized
# containers
if [[ -n "$RESUME" ]]; then
   WORK_DIR=$RESUME
   test -d $WORK_DIR || error_exit "$WORD_DIR does not exist, cannot resume."
else
   WORK_DIR=`mktemp -d canoe-parallel.XXX` || error_exit "Cannot create temp workdir."
fi
INPUT="$WORK_DIR/input"
CMDS_FILE=$WORK_DIR/commands

# Make sure there is canoe on the command line.
[[ $GOTCANOE ]] || error_exit "The 'canoe' argument and the canoe options are missing."

# -j is only for byblock load balancing (-lb option)
if [[ $NUMBER_OF_JOBS -ne 1 && $LOAD_BALANCING != byblock ]]; then
   warn "-j(ob) option is only available for regular load-balancing (-lb)"
fi


# Make sure the path to self is on the PATH variable: if an explicit path to
# canoe-parallel.sh is given, we want to look for canoe and other programs in
# the same directory, rather than find it on the path.
export PATH=`dirname "$0"`:$PATH

if [[ $VERBOSE -gt 0 ]]; then
   echo "" >&2
   echo Starting $$ on `hostname` on `date` >&2
   echo $0 $SAVE_ARGS >&2
   echo Using `which $CANOE` >&2
   echo "" >&2
   if [[ $LOAD_BALANCING == bysent ]]; then
      echo "Using by-sentence load balancing" >&2
   elif [[ $LOAD_BALANCING ]]; then
      echo "Using load balancing" >&2
   else
      echo "NOT using load balancing" >&2
   fi
fi

# When using TPLMs and TPPTs, we need to request additional virtual memory
# allowance for memory mapped IO.
   FILE_PATTERN="-f  *([^ ][^ ]*)"
   if [[ "${CANOEOPTS[*]}" =~ ${FILE_PATTERN} ]]; then
      CONFIGFILE=${BASH_REMATCH[1]}
   else
      CONFIGFILE=canoe.ini
   fi
   debug CONFIGFILE=$CONFIGFILE
   if (configtool check $CONFIGFILE > /dev/null); then true; else
      error_exit "Problem with config file $CONFIGFILE."
   fi
   MEMMAP_SIZE=`configtool memmap $CONFIGFILE`
   debug MEMMAP_SIZE=$MEMMAP_SIZE
   if [ $VERBOSE -gt 0 ]; then
      echo "Using memory-mapped files totaling "$(bc <<< "scale=3; $MEMMAP_SIZE / 1024")"G" >&2
   fi
   if [[ $MEMMAP_SIZE -ge 2048 ]]; then
      PSUBOPTS="-memmap $((MEMMAP_SIZE / 1024 / 2)) $PSUBOPTS"
   fi

if [ $DEBUG ]; then
   for f in "${CANOEOPTS[@]}"; do
     echo $f
   done
   echo "
   NUM=$NUM
   PSUBOPTS=$PSUBOPTS
   QSUBOPTS=$QSUBOPTS
   RUN_PARALLEL_OPTS=$RUN_PARALLEL_OPTS
   CANOEOPTS=${CANOEOPTS[*]} ("${#CANOEOPTS[*]}")
   VERBOSE=$VERBOSE
   DEBUG=$DEBUG
   LOAD_BALANCING=$LOAD_BALANCING
   RESUME=$RESUME
"
fi >&2


if [ -n "$PSUBOPTS" ]; then
   RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -psub \"$PSUBOPTS\""
fi
if [ -n "$QSUBOPTS" ]; then
   RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -qsub \"$QSUBOPTS\""
fi
debug "RUN_PARALLEL_OPTS=$RUN_PARALLEL_OPTS"

function create_hierarchy
{
   HIERARCHY_SIZE=$1
   # Let's create the directory hierarchy.
   # This is to prevent race conditions between workers.
   if [[ $NBEST_PREFIX ]]; then
      DIRNAME=`dirname $NBEST_PREFIX`
      echo "Creating hierarchy $DIRNAME/%03.f" >&2
      mkdir -p `seq --format="$DIRNAME/%03.f" 0 $(($HIERARCHY_SIZE/1000))` \
      || error_exit "Unable to create the directory hierarchy."
   fi
   if [[ $LATTICE_PREFIX ]] && [[ $LATTICE_PREFIX != $NBEST_PREFIX ]]; then
      DIRNAME=`dirname $LATTICE_PREFIX`
      echo "Creating hierarchy $DIRNAME/%03.f" >&2
      mkdir -p `seq --format="$DIRNAME/%03.f" 0 $(($HIERARCHY_SIZE/1000))` \
      || error_exit "Unable to create the directory hierarchy."
   fi
}

function full_translation
{
   # Calculate the number of jobs/workers to use if user hasn't specified it,
   min () { if [ $1 -lt $2 ]; then echo $1; else echo $2; fi }
   if [ ! $NUM ]; then
      if on-cluster.sh; then
         NUM=$(( ($INPUT_LINES + $HIGH_SENT_PER_BLOCK - 1) / $HIGH_SENT_PER_BLOCK ))
         if [ $NUM -lt $MIN_DEFAULT_NUM ]; then
               NUM=$(min $MIN_DEFAULT_NUM $(( ($INPUT_LINES + $LOW_SENT_PER_BLOCK - 1) / $LOW_SENT_PER_BLOCK )) )
         elif [ $NUM -gt $MAX_DEFAULT_NUM ]; then
               NUM=$MAX_DEFAULT_NUM
         fi
      else
         if [[ $(uname -s) != Darwin ]]; then
            NUM=$(echo `grep processor /proc/cpuinfo | wc -l`)
         else
            NUM=$(sysctl -n hw.ncpu)
         fi
         # Be friendly by default: use half the CPUs on a machine!
         NUM=$((($NUM + 1) / 2))
         if [ $NUM -lt 1 ]; then NUM=1; fi
      fi

      debug "NUM=$NUM"
   fi
   if [ ! $PARNUM ]; then PARNUM=$NUM; fi

   # Calculate the number of lines per block
   LINES_PER_BLOCK=$(( ( $INPUT_LINES + $NUM - 1 ) / $NUM ))
   debug "LINES_PER_BLOCK=$LINES_PER_BLOCK"

   # the commands file will have the list of commands to execute
   CMDS_FILE=$WORK_DIR/commands
   test -f "$CMDS_FILE" && rm -f $CMDS_FILE

   # LOAD BALANCING mode
   if [[ $LOAD_BALANCING ]]; then
      # Make sure there are at least as many jobs as there are workers
      if [ $NUM -gt $NUMBER_OF_JOBS ]; then
         NUMBER_OF_JOBS=$NUM
      fi

      if [[ $LOAD_BALANCING == byblock ]]; then
         # Load balancing command
         LB_CMD="cat $INPUT | load-balancing.pl -output="$INPUT" -node=$NUM -job=$NUMBER_OF_JOBS"
         test -n "$REF" && LB_CMD="$LB_CMD -ref=$REF"
         debug "load-balancing: $LB_CMD"
         eval "$LB_CMD"
         RC=$?
         if ((RC != 0)); then
            echo "Error spliting the input for load-balancing" >&2
            exit -7;
         fi
      fi

      # Canoe cannot do load-balancing and append at the same time, so
      # we remove -append here and post-process the output at the end of this
      # script to perform the -append operation.
      declare -a CONFIGARG
      for i in ${!CANOEOPTS[@]}; do
         if [[ ${CANOEOPTS[$i]} =~ " " ]]; then
            # Quote arguments with spaces.
            CONFIGARG=("${CONFIGARG[@]}" "\"${CANOEOPTS[$i]//\"/\\\"}\"")
         else
            # Remove -append
            if [[ ${CANOEOPTS[$i]} =~ "-append" ]]; then
               true
            else
               CONFIGARG=("${CONFIGARG[@]}" ${CANOEOPTS[$i]})
            fi
         fi
      done

      [[ $HIERARCHY ]] && create_hierarchy $INPUT_LINES

      if [[ $LOAD_BALANCING == byblock ]]; then
         # regular load balancing: by blocks
         for (( i = 0 ; i < $NUMBER_OF_JOBS ; ++i )); do
            CANOE_INPUT=`printf "$INPUT.%4.4d" $i`
            OUT=$WORK_DIR/out.$i
            ERR=$WORK_DIR/err.$i
            CMD="time $CANOE ${CONFIGARG[@]} -lb"
            test -n "$REF" && CMD="$CMD -ref "`printf "$INPUT.ref.%4.4d" $i`
            CMD="cat $CANOE_INPUT | $CMD > $OUT 2> $ERR"

            # add to commands file or run
            debug "LB canoe cmd: $CMD"
            echo "test -f $CANOE_INPUT.done || ($CMD && mv $CANOE_INPUT ${CANOE_INPUT}.done)"
         done > $CMDS_FILE
      else
         # load balancing sentence by sentence
         seq 0 $((INPUT_LINES - 1)) > $CMDS_FILE
         WORKER_CMD="$CANOE ${CONFIGARG[@]} -input $INPUT -canoe-daemon __HOST__:__PORT__"
      fi
   # NOT in load-balancing.
   else
      # When running in append mode we need to create intermediate files that
      # we need to keep track of
      # Remove the -nbest option from the list
      CANOEOPTS_APPEND=`echo "${CANOEOPTS[*]}" | perl -pe 's/(^| )-nbest \S+/ /'`
      # Remove the -lattice option from the list
      CANOEOPTS_APPEND=`echo "${CANOEOPTS_APPEND[*]}" | perl -pe 's/(^| )-lattice \S+/ /'`
      debug "CANOEOPTS_APPEND: ${CANOEOPTS_APPEND[*]}"

      [[ $HIERARCHY ]] && create_hierarchy $INPUT_LINES

      # Run or prepare all job blocks
      for (( i = 0 ; i < $NUM ; ++i )); do
         if [ $INPUT_LINES -le $(( (i + 1) * LINES_PER_BLOCK)) ]; then
            # input file small - this block will cover all remaining lines
            NUM=$((i + 1))
         fi

         # set up the canoe command string
         CONFIGARG=
            FIRST_SENT_NUM=$(( $i * $LINES_PER_BLOCK ))

         CANOE_CMD="time $CANOE ${CANOEOPTS_APPEND[*]} $CONFIGARG -first-sentnum $FIRST_SENT_NUM"
         PRECISION=${PRECISION:-3}
         [[ $APPEND ]] && INDEX=`printf ".%${PRECISION}.${PRECISION}d" $i`   # Build the chunk's index
         [[ $NBEST_PREFIX ]] && CANOE_CMD="$CANOE_CMD -nbest ${NBEST_PREFIX}${INDEX}${NBEST_COMPRESS}${NBEST_SIZE}"
         [[ $LATTICE_PREFIX ]] && CANOE_CMD="$CANOE_CMD -lattice ${LATTICE_PREFIX}${INDEX}${LATTICE_COMPRESS}"
         debug "CANOE_CMD $i=$CANOE_CMD"
         CANOE_INPUT=$INPUT.$i
         OUT=$WORK_DIR/out.$i
         ERR=$WORK_DIR/err.$i

         # select the input to feed to this job
         SELECT_LINES_CMD_R=""

            if [ $i -lt $((NUM - 1)) ]; then
               LAST_LINE=$(( (i + 1) * LINES_PER_BLOCK ))
               head -$LAST_LINE $INPUT | tail -$LINES_PER_BLOCK > $CANOE_INPUT
               if [ $REF ]; then
                  SELECT_LINES_CMD_R="-ref \"gzip -cdqf < $REF | head -$LAST_LINE | tail -$LINES_PER_BLOCK |\""
               fi
            else
               LINES_IN_LAST_BLOCK=$(( INPUT_LINES - i * LINES_PER_BLOCK ))
               tail -$LINES_IN_LAST_BLOCK $INPUT > $CANOE_INPUT
               if [ $REF ]; then
                  SELECT_LINES_CMD_R="-ref \"gzip -cdqf < $REF | tail -$LINES_IN_LAST_BLOCK |\""
               fi
            fi

         CMD="cat $CANOE_INPUT | $CANOE_CMD $SELECT_LINES_CMD_R > $OUT 2> $ERR"

         # add to commands file or run
         # NOTE that if the block is successful we flag it at is by deleting its input
         echo "test -f $CANOE_INPUT.done || ($CMD && mv $CANOE_INPUT ${CANOE_INPUT}.done)"
      done > $CMDS_FILE
   fi
}

# Read the input file and save it to tmp so we can manipulate it as needed.
cat > $INPUT

# This seemingly strange use of echo remove the whitespace around the output
# of wc.  Though not strictly necessary, it makes things cleaner for the
# remaining of the code.
INPUT_LINES=$(echo `wc -l < $INPUT`)

debug "INPUT_LINES=$INPUT_LINES"
if [ $INPUT_LINES -eq 0 ]; then
   echo No input data from STDIN, nothing to do\! >&2
   exit 0
fi;

# Get the -nbest option from the list
NBEST_PREFIX=`echo "${CANOEOPTS[*]}" | perl -ne '/(^| )-nbest (\S+)/o; print $2'`
debug "NBEST_PREFIX: $NBEST_PREFIX"

# Extract/remove the nbest list size
TEMP=$NBEST_PREFIX
NBEST_PREFIX=${TEMP%:*}
NBEST_SIZE=${TEMP#$NBEST_PREFIX}

# Extract/remove the compression from the nbest list
TEMP=$NBEST_PREFIX
NBEST_PREFIX=${TEMP%.gz}
NBEST_COMPRESS=${TEMP#$NBEST_PREFIX}

# Get the -lattice argument prefix and optional .gz suffix
TEMP=`echo "${CANOEOPTS[*]}" | perl -ne '/(^| )-lattice (\S+)/o; print $2'`
LATTICE_PREFIX=${TEMP%.gz}
LATTICE_COMPRESS=${TEMP#$LATTICE_PREFIX}

debug "NBEST_PREFIX:$NBEST_PREFIX"
debug "NBEST_SIZE: $NBEST_SIZE"
debug "NBEST_COMPRESS: $NBEST_COMPRESS"

# Are we using hierarchy?
[[ "${CANOEOPTS[*]}" =~ "(^| )-hierarchy" ]] && HIERARCHY=" -hierarchy"
debug "HIERARCHY:$HIERARCHY"
[[ $APPEND ]] && [[ $HIERARCHY ]] && error_exit "-hierarchy and -append mode are incompatible."

# Autoresume of failed jobs or to translation
if [ -n "$RESUME" ]; then
   debug "Resuming job: $RESUME"
   test -f "$CMDS_FILE" || error_exit "Cannot resume with file $CMDS_FILE."

   if [ ! -s "$CMDS_FILE" ]; then
      warn "Every chunks seem complete will try to merge them"
   fi
else
   full_translation;
fi

if [ ! -s $CMDS_FILE ]; then
   error_exit "There is no command to run."
fi
if [ ! $PARNUM ]; then PARNUM=$NUM; fi


# Submit the jobs to run-parallel.sh, which will take care of parallizing them
if [[ $WORKER_CMD ]]; then
   WORKER_ARG="-worker-cmd \"$WORKER_CMD\" -unordered-cat"
   debug "run-parallel.sh $WORKER_ARG $RUN_PARALLEL_OPTS $CMDS_FILE $PARNUM > $WORK_DIR/out.unordered"
   eval run-parallel.sh $WORKER_ARG $RUN_PARALLEL_OPTS $CMDS_FILE $PARNUM > $WORK_DIR/out.unordered
else
   debug "run-parallel.sh $RUN_PARALLEL_OPTS $CMDS_FILE $PARNUM"
   eval run-parallel.sh $RUN_PARALLEL_OPTS $CMDS_FILE $PARNUM
fi
RC=$?
if (( $RC != 0 )); then
   echo "problems with run-parallel.sh(RC=$RC) - quitting!" >&2
   echo "The temp output files will not be merged or deleted to give" \
        "the user a chance to salvage as much data as possible." >&2
   exit 4
fi


# Reassemble the program's STDOUT
if [[ $LOAD_BALANCING ]]; then
   # Sort translation by src sent id and then remove id
   echo "Rebuilding STDOUT" >&2
   time { cat $WORK_DIR/out.* | sort -n | cut -f2; }
   TOTAL_LINES_OUTPUT=`cat $WORK_DIR/out.* | wc -l`
else
   TOTAL_LINES_OUTPUT=0
   for (( i = 0; i < $NUM ; ++i )); do
      cat $WORK_DIR/out.$i
      TOTAL_LINES_OUTPUT=$(($TOTAL_LINES_OUTPUT + `wc -l < $WORK_DIR/out.$i`))
   done
fi

# Reassemble the program's STDERR
if [ $VERBOSE -gt 0 ]; then
   for f_err in `ls -v $WORK_DIR/err.* 2> /dev/null`;
   do
      i=`echo "$f_err" | egrep -o "[0-9]*$"`
      echo "" >&2
      echo "==> block $i output" `wc -l < ${f_err/$WORK_DIR\/err/$WORK_DIR\/out}` "lines <==" >&2
      echo "==> STDERR from block $i <==" >&2
      cat $f_err >&2
   done
fi

if [ $VERBOSE -gt 0 ]; then
   echo "" >&2
   echo Done $$ on `hostname` on `date` >&2
   echo "" >&2
fi


# Data consistency check.
# Canoe must generate as many output translations as input sentences.
if [ $TOTAL_LINES_OUTPUT -ne $INPUT_LINES ]; then
   echo "Missing some output lines: expected $INPUT_LINES," \
        "got $TOTAL_LINES_OUTPUT" >&2
   echo "The temp output files will not be merged or deleted, to give" \
        "the user a chance to salvage as much data as possible." >&2
   exit 1
fi


# Merging output chunks (nbest, ffvals, sfvals, pal, and lattice)
if [ -n "$APPEND" ]; then
   FFVALS_CREATED=`echo ${CANOEOPTS[*]} | egrep -oe '-ffvals'`
   SFVALS_CREATED=`echo ${CANOEOPTS[*]} | egrep -oe '-sfvals'`
   PAL_CREATED=`echo ${CANOEOPTS[*]} | egrep -oe '-palign' -e '-t ' -e '-trace'`

   K=${NBEST_SIZE#:}
   K=${K:-100}
   if [ -n "$LOAD_BALANCING" ]; then
      NUM_MERGE=$INPUT_LINES
   else
      NUM_MERGE=$NUM
   fi

   ### concatenating separate lists into one big file
   DIRNAME=`dirname ${NBEST_PREFIX}`
   BASENAME=`basename ${NBEST_PREFIX}`

   # merge ffvals files
   if [ -n "$FFVALS_CREATED" ] && [ -n "$NBEST_PREFIX" ]; then
      [[ `find $DIRNAME -name $BASENAME.\*.${K}best.ffvals$NBEST_COMPRESS | \wc -l` -eq $NUM_MERGE ]] \
      || error_exit "There are some missing ffvals files."

      OUTPUT="${NBEST_PREFIX}ffvals$NBEST_COMPRESS"
      debug "LB FFVALS output: $OUTPUT"
      test -f $OUTPUT && \rm $OUTPUT
      time { find $DIRNAME -name $BASENAME.\*.${K}best.ffvals$NBEST_COMPRESS \
         | sed "s/\(.\+\.\([0-9]\+\)\.${K}best.ffvals$NBEST_COMPRESS\)/\2\t\1/" \
         | LC_ALL=C sort -g -k1,1 \
         | cut -f2- \
         | xargs --no-run-if-empty cat > $OUTPUT; }
      [[ $DEBUG ]] || find $DIRNAME -name $BASENAME.\*.${K}best.ffvals$NBEST_COMPRESS | xargs \rm
   fi

   # merge sfvals files
   if [ -n "$SFVALS_CREATED" ] && [ -n "$NBEST_PREFIX" ]; then
      [[ `find $DIRNAME -name $BASENAME.\*.${K}best.sfvals$NBEST_COMPRESS | \wc -l` -eq $NUM_MERGE ]] \
      || error_exit "There are some missing sfvals files."

      OUTPUT="${NBEST_PREFIX}sfvals$NBEST_COMPRESS"
      debug "LB SFVALS output: $OUTPUT"
      test -f $OUTPUT && \rm $OUTPUT
      time { find $DIRNAME -name $BASENAME.\*.${K}best.sfvals$NBEST_COMPRESS \
         | sed "s/\(.\+\.\([0-9]\+\)\.${K}best.sfvals$NBEST_COMPRESS\)/\2\t\1/" \
         | LC_ALL=C sort -g -k1,1 \
         | cut -f2- \
         | xargs --no-run-if-empty cat > $OUTPUT; }
      [[ $DEBUG ]] || find $DIRNAME -name $BASENAME.\*.${K}best.sfvals$NBEST_COMPRESS | xargs \rm
   fi

   # merge pal files
   if [ -n "$PAL_CREATED" ] && [ -n "$NBEST_PREFIX" ]; then
      [[ `find $DIRNAME -name $BASENAME.\*.${K}best.pal$NBEST_COMPRESS | \wc -l` -eq $NUM_MERGE ]] \
      || error_exit "There are some missing pal files."

      OUTPUT="${NBEST_PREFIX}pal$NBEST_COMPRESS"
      debug "LB PAL output: $OUTPUT"
      test -f $OUTPUT && \rm $OUTPUT
      time { find $DIRNAME -name $BASENAME.\*.${K}best.pal$NBEST_COMPRESS \
         | sed "s/\(.\+\.\([0-9]\+\)\.${K}best.pal$NBEST_COMPRESS\)/\2\t\1/" \
         | LC_ALL=C sort -g -k1,1 \
         | cut -f2- \
         | xargs --no-run-if-empty cat > $OUTPUT; }
      [[ $DEBUG ]] || find $DIRNAME -name $BASENAME.\*.${K}best.pal$NBEST_COMPRESS | xargs \rm
   fi

   # merge the nbest files
   if [ -n "$NBEST_PREFIX" ]; then
      [[ `find $DIRNAME -name $BASENAME.\*.${K}best$NBEST_COMPRESS | \wc -l` -eq $NUM_MERGE ]] \
      || error_exit "There are some missing Nbest files."

      OUTPUT="${NBEST_PREFIX}nbest$NBEST_COMPRESS"
      debug "LB NBEST output: $OUTPUT"
      test -f $OUTPUT && \rm $OUTPUT
      time { find $DIRNAME -name $BASENAME.\*.${K}best$NBEST_COMPRESS \
         | sed "s/\(.\+\.\([0-9]\+\)\.${K}best$NBEST_COMPRESS\)/\2\t\1/" \
         | LC_ALL=C sort -g -k1,1 \
         | cut -f2- \
         | xargs --no-run-if-empty cat > $OUTPUT; }
      [[ $DEBUG ]] || find $DIRNAME -name $BASENAME.\*.${K}best$NBEST_COMPRESS | xargs \rm
   fi

   #[[ $DEBUG ]] || rm -r $DIRNAME

   # merge lattice and lattice state files
   if [ -n "$LATTICE_PREFIX" ]; then
      DIRNAME=`dirname ${LATTICE_PREFIX}`
      BASENAME=`basename ${LATTICE_PREFIX}`

      [[ `find $DIRNAME -name $BASENAME.\*[0-9]$LATTICE_COMPRESS | \wc -l` -eq $NUM_MERGE ]] \
      || error_exit "There are some missing lattice files."

      OUTPUT="${LATTICE_PREFIX}$LATTICE_COMPRESS"
      test -f $OUTPUT && \rm $OUTPUT
      time { find $DIRNAME -name $BASENAME.\*[0-9]$LATTICE_COMPRESS \
         | sed "s/\(.\+\.\([0-9]\+\)$LATTICE_COMPRESS\)/\2\t\1/" \
         | LC_ALL=C sort -g -k1,1 \
         | cut -f2- \
         | xargs --no-run-if-empty cat > $OUTPUT; }
      [[ $DEBUG ]] || find $DIRNAME -name $BASENAME.\*[0-9]$LATTICE_COMPRESS | xargs \rm

      # Concatenate only if there are some state files.
      LATTICE_STATE_COUNT=`find $DIRNAME -name $BASENAME.\*[0-9].state$LATTICE_COMPRESS | \wc -l`
      if [[ $LATTICE_STATE_COUNT -gt 0 ]]; then
         [[ $LATTICE_STATE_COUNT -eq $NUM_MERGE ]] || error_exit "There are some missing lattice state files."

         OUTPUT="${LATTICE_PREFIX}.state$LATTICE_COMPRESS"
         test -f $OUTPUT && \rm $OUTPUT
         time { find $DIRNAME -name $BASENAME.\*[0-9].state$LATTICE_COMPRESS \
            | sed "s/\(.\+\.\([0-9]\+\).state$LATTICE_COMPRESS\)/\2\t\1/" \
            | LC_ALL=C sort -g -k1,1 \
            | cut -f2- \
            | xargs --no-run-if-empty cat > $OUTPUT; }
         [[ $DEBUG ]] || find $DIRNAME -name $BASENAME.\*[0-9].state$LATTICE_COMPRESS | xargs \rm
      fi

      #[[ $DEBUG ]] || rm -r $DIRNAME
   fi
fi

# Everything went fine, clean up
[[ -e $WORK_DIR ]] && rm -rf $WORK_DIR
exit 0;
