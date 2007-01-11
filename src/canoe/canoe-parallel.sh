#!/bin/bash
# $Id$
# canoe-parallel.sh - wrapper to run canoe on a parallel machine or on a
# cluster.
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

PATH="$PATH:/usr/local/bin"

CANOE=canoe
MIN_DEFAULT_NUM=4
MAX_DEFAULT_NUM=10
OUTPUT_FILE_PATTERN=canoe-parallel-output.$$
INPUT=canoe-parallel-input.$$
HIGH_SENT_PER_BLOCK=100
LOW_SENT_PER_BLOCK=40


usage() {
    for msg in "$@"; do
        echo $msg >&2
    done
    cat <<==EOF== >&2
canoe-parallel.sh, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada

Usage: canoe-parallel.sh [options] canoe [canoe options] < <input>

  Divides the input into several blocks and runs canoe in parallel on each
  of them.

  The input file is divided in equal sized blocks which are each given to a
  separate instance of canoe with the same options, and the final output of
  each block is merged together so that the output of canoe-parallel.sh is
  identical to the output of canoe with the same options.

Options:

  -c(luster):   uses the 'run-parallel.sh' command to submit each block on
                different nodes of the cluster.  [default: on the cluster, run
                in cluster mode, not on the cluster, run each block in the
                background, assuming a multi-CPU machine]

  -noc(luster): turns off cluster mode [default: see -cluster option]

  -n(um) N:     split the input into N blocks.  [default: number of CPUs in
                single multi-CPU machine mode; # input sentences / $HIGH_SENT_PER_BLOCK,
                but >= $MIN_DEFAULT_NUM and <= $MAX_DEFAULT_NUM in cluster mode]

  -h(elp):      print this help message

  -v(erbose):   increment the verbosity level by 1 (may be repeated)
                [default: verbosity level 1]
                Note that this is not the same as canoe's -v option, which
                should be provided *after* the "canoe" keyword if desired.

  -q(quiet):    suppress all verbose output, including verbose output by canoe

  -d(ebug):     turn the debug mode on.

  canoe <canoe options>: this mandatory argument must occur after all
                options to this script and before all regular canoe options.

Cluster mode options:

  -nolowpri:    do not use the "low priority queue".  Needed if your
                resource requirements will only be met on our nodes.
                [default: use the "low priority queue".]

  -highmem:     allocate 2 cpus per job, for extra extra memory.
                (Implies -nolowpri.)

  -nolocal:     by default, one of the blocks will be executed directly
                rather than submitted, so that the node this script is
                running on is not left idle waiting for the other blocks to
                finish.  -nolocal forces the use of the queues for all
                blocks.

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


# error_exit "some error message" "optionnally a second line of error message"
# will exit with an error status, print the specified error message(s) on
# STDERR.
error_exit() {
    for msg in "$@"; do
        echo $msg >&2
    done
    echo "Use -h for help." >&2
    exit 1
}

# Verify that enough args remain on the command line
# syntax: one_arg_check <args needed> $# <arg name>
# Note that this function expects to be in a while/case structure for
# handling parameters, so that $# still includes the option itself.
# exits with error message if the check fails.
arg_check() {
    if [ $2 -le $1 ]; then
        error_exit "Missing argument to $3 option."
    fi
}

# Command line processing ; "declare -a" declares array variables
declare -a CANOEOPTS
RUN_PARALLEL_OPTS=
CLUSTER=
NUM=
VERBOSE=1
DEBUG=
GOTCANOE=
SAVE_ARGS="$@"
while [ $# -gt 0 ]; do
    case "$1" in
    -c|-cluster)    CLUSTER=1;;
    -noc|-nocluster)CLUSTER=0;;
    -n|-num)        arg_check 1 $# $1; NUM=$2; shift;;
    -nolowpri)      RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
    -highmem)       RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
    -nolocal)       RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
    -psub|-psub-opts|-psub-options)
                    arg_check 1 $# $1; PSUBOPTS="$PSUBOPTS $2"; shift;;
    -qsub|-qsub-opts|-qsub-options)
                    arg_check 1 $# $1; QSUBOPTS="$QSUBOPTS $2"; shift;;
    -v|-verbose)    VERBOSE=$(( $VERBOSE + 1 ))
                    RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
    -q|-quiet)      VERBOSE=0
                    RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS $1";;
    -d|-debug)      DEBUG=1;;
    -h|-help)       usage;;
    canoe)          shift; GOTCANOE=1; CANOEOPTS=("$@"); break;;
    *)              error_exit "Unknown option $1." \
                    "Possibly missing the 'canoe' argument before regular canoe options?"
                    ;;
    esac
    shift
done

if [ -z "$CLUSTER" ]; then
    # neither -cluster nor -nocluster was specified, so turn cluster mode on
    # if we're running on a cluster.  We assume we're on a cluster if the
    # qsub command exists and is executable.
    if which-test.sh qsub; then
        CLUSTER=1
    else
        CLUSTER=0
    fi
fi

# Make sure the path to self is on the PATH variable: if an explicit path to
# canoe-parallel.sh is given, we want to look for canoe and other programs in
# the same directory, rather than find it on the path.
export PATH=`dirname $0`:$PATH

if [ $VERBOSE -gt 0 ]; then
    echo "" >&2
    echo Starting $$ on `hostname` on `date` >&2
    echo $0 $SAVE_ARGS >&2
    echo Using `which $CANOE nbest2rescore.pl` >&2
    echo "" >&2
fi

if [ $DEBUG ]; then
    echo "
    CLUSTER=$CLUSTER
    NUM=$NUM
    PSUBOPTS=$PSUBOPTS
    QSUBOPTS=$QSUBOPTS
    RUN_PARALLEL_OPTS=$RUN_PARALLEL_OPTS
    CANOEOPTS=${CANOEOPTS[*]} ("${#CANOEOPTS[*]}")
    VERBOSE=$VERBOSE
    DEBUG=$DEBUG
" >&2
fi

if [ $CLUSTER == 0 ]; then
    test $RUN_PARALLEL_OPTS && echo "Ignoring irrelevant option(s) $RUN_PARALLEL_OPTS" >&2
    test "$PSUBOPTS" && echo "Ignoring irrelevant option -psub-options" >&2
    test "$QSUBOPTS" && echo "Ignoring irrelevant option -qsub-options" >&2
else
    if [ -n "$PSUBOPTS" ]; then
        RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -psub \"$PSUBOPTS\""
    fi
    if [ -n "$QSUBOPTS" ]; then
        RUN_PARALLEL_OPTS="$RUN_PARALLEL_OPTS -qsub \"$QSUBOPTS\""
    fi
    if [ $DEBUG ]; then
        echo "    RUN_PARALLEL_OPTS=$RUN_PARALLEL_OPTS" >&2
    fi
fi

if [ ! $GOTCANOE ]; then
    error_exit "The 'canoe' argument and the canoe options are missing"
fi

# Setup some clean up.  When we end or die:
#  - we want to delete temp files
#  - we want to kill children that haven't terminated, so they don't just
#    keep running
#    -> This doesn't work correctly because we use "time" to get running
#       stats about canoe, and time doesn't forward the kill signals to
#       its children.  Too bad... :(
trap '
    if [ -n "$CHILDREN_PROCESSES" ]; then
        kill $CHILDREN_PROCESSES
    fi
    rm -f psub-dummy-output $INPUT $OUTPUT_FILE_PATTERN.*
' 0 2 3 13 15
#' 9

# Read the input file and save it to tmp so we can manipulate it as needed.
cat > $INPUT

# This seemingly strange use of echo remove the whitespace around the output
# of wc.  Though not strictly necessary, it makes things cleaner for the
# remaining of the code.
INPUT_LINES=$(echo `wc -l < $INPUT`)

test $DEBUG && echo "    INPUT_LINES=$INPUT_LINES" >&2
if [ $INPUT_LINES -eq 0 ]; then
    echo No input data from STDIN, nothing to do\! >&2
    exit 0
fi;

# Calculate the number of nodes/CPUs to use
min () { if [ $1 -lt $2 ]; then echo $1; else echo $2; fi }
if [ ! $NUM ]; then
    if [ $CLUSTER == 1 ]; then
        NUM=$(( ($INPUT_LINES + $HIGH_SENT_PER_BLOCK - 1) / $HIGH_SENT_PER_BLOCK ))
        if [ $NUM -lt $MIN_DEFAULT_NUM ]; then
            NUM=$(min $MIN_DEFAULT_NUM $(( ($INPUT_LINES + $LOW_SENT_PER_BLOCK - 1) / $LOW_SENT_PER_BLOCK )) )
        elif [ $NUM -gt $MAX_DEFAULT_NUM ]; then
            NUM=$MAX_DEFAULT_NUM
        fi
    else
        NUM=$(echo `grep processor /proc/cpuinfo | wc -l`)
        # Be friendly by default: use half the CPUs on a machine!
        NUM=$((($NUM + 1) / 2))
        if [ $NUM -lt 1 ]; then NUM=1; fi
    fi

    test $DEBUG && echo "    NUM=$NUM" >&2
fi

# Calculate the number of lines per block
LINES_PER_BLOCK=$(( ( $INPUT_LINES + $NUM - 1 ) / $NUM ))
test $DEBUG && echo "    LINES_PER_BLOCK=$LINES_PER_BLOCK" >&2

# the .commands file will have the list of commands to execute
CMDS_FILE=$OUTPUT_FILE_PATTERN.commands
if [ $CLUSTER == 1 ]; then
    cat /dev/null > $CMDS_FILE
fi

# Run or prepare all job blocks
for (( i = 0 ; i < $NUM ; ++i )); do
    if [ $INPUT_LINES -le $(( (i + 1) * LINES_PER_BLOCK)) ]; then
        # input file small - this block will cover all remaining lines
        NUM=$((i + 1))
    fi
    FIRST_SENT_NUM=$(( $i * $LINES_PER_BLOCK ))
    CANOE_CMD="time $CANOE ${CANOEOPTS[*]} -first-sentnum $FIRST_SENT_NUM"
    #test $DEBUG && echo "    CANOE_CMD $i=$CANOE_CMD" >&2
    OUT=$OUTPUT_FILE_PATTERN.out.$i
    ERR=$OUTPUT_FILE_PATTERN.err.$i
    if [ $i -lt $((NUM - 1)) ]; then
        LAST_LINE=$(( (i + 1) * LINES_PER_BLOCK ))
        SELECT_LINES_CMD="head -$LAST_LINE $INPUT | tail -$LINES_PER_BLOCK"
    else
        LINES_IN_LAST_BLOCK=$(( INPUT_LINES - i * LINES_PER_BLOCK ))
        SELECT_LINES_CMD="tail -$LINES_IN_LAST_BLOCK $INPUT"
    fi
    CMD="$SELECT_LINES_CMD | $CANOE_CMD > $OUT 2> $ERR"
    if [ $CLUSTER == 1 ]; then
        echo "$CMD" >> $CMDS_FILE
    else
        test $VERBOSE -gt 0 && echo "$CMD" >&2
        eval $CMD &
        CHILDREN_PROCESSES="$CHILDREN_PROCESSES $!"
    fi
done

if [ $CLUSTER == 1 ]; then
    # In cluster mode, submit the workers on the constructed command file
    eval run-parallel.sh $RUN_PARALLEL_OPTS $CMDS_FILE $NUM
else
    # In local mode, wait for all blocks to finish
    wait
    CHILDREN_PROCESSES=""
fi

# Reassemble the program's STDOUT and STDERR
TOTAL_LINES_OUTPUT=0
for (( i = 0; i < $NUM ; ++i )); do
    cat $OUTPUT_FILE_PATTERN.out.$i
    TOTAL_LINES_OUTPUT=$(($TOTAL_LINES_OUTPUT + `wc -l < $OUTPUT_FILE_PATTERN.out.$i`))

    if [ $VERBOSE -gt 0 ]; then
        echo "" >&2
        echo "==> block $i output" `wc -l < $OUTPUT_FILE_PATTERN.out.$i` "lines <==" >&2
        echo "==> STDERR from block $i <==" >&2
        cat $OUTPUT_FILE_PATTERN.err.$i >&2
    fi
done

if [ $VERBOSE -gt 0 ]; then
    echo "" >&2
    echo Done $$ on `hostname` on `date` >&2
    echo "" >&2
fi

if [ $TOTAL_LINES_OUTPUT -ne $INPUT_LINES ]; then
   exit 1
fi
