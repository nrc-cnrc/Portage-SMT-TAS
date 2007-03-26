#!/bin/bash
# $Id$

# run-parallel.sh - runs a series of jobs provided as STDIN on parallel
#                   distributed workers managed by faucet.pl / worker.pl.
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

echo 'run-parallel.sh, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada' >&2

usage() {
    for msg in "$@"; do
        echo -- $msg >&2
    done
    cat <<==EOF== >&2

Usage: run-parallel.sh [options] <file_of_commands> <N>

  Read a series of commands from STDIN and execute them on <N> parallel
  workers.  When running on a cluster, each worker is launched through psub.
  Otherwise, the workers are launched as background jobs.

  For optimal performance, uses faucet and netcat to keep <N> CPUs constantly
  busy until all jobs are completed.  Without faucet or netcat, jobs are simply
  launched in the background <N> at a time, and each group has to finish
  completely before the next group is launched.

Arguments:

  <file_of_commands>  A file with exactly one command per line, in valid
      bash syntax.  (Use - for STDIN.) Each command may include redirections
      and pipes, will run in the current directory, and will be aware of the
      current value of PATH.  Commands should not depend on each other, since
      they might all start at the same time.  Each command should explicitly
      redirect its STDOUT and STDERR, otherwise their output will be all
      together in the STDERR stream of this script.

  <N> Number of workers to launch (need not be the same as the number of
      commands in <file_of_commands>)

General options:

  -h(elp)       Print this help message.
  -d(ebug)      Print debugging information.
  -quiet        Quiet mode only prints commands executed.

Cluster mode options:

  -nolowpri     Do not use the "low priority queue" (useful if you need
                extra memory)
  -highmem      Use 2 CPUs per worker, for extra extra memory.
                (Implies -nolowpri.)
  -nolocal      psub all workers (by default, one worker is run locally)
  -quota T      When workers have done T minutes of work, re-psub them [60]
  -p(sub)       Provide custom psub options.
  -q(sub)       Provide custom qsub options.

Dynamic options:

  To add N workers on the fly, run this command in the directory where the
  <long_prefix>.todo file is located:
                echo N > <long_prefix>.add

  To quench the process and remove N workers on the files, run this command:
                echo N > <long_prefix>.quench

  Quench and add requests are processed the next time the faucet is awakened by
  a worker, so it may take some time before they have any effects.

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

NUM=
NOLOWPRI=
HIGHMEM=
NOLOCAL=
VERBOSE=1
DEBUG=
SAVE_ARGS="$@"
CMD_FILE=
QUOTA=
while [ $# -gt 0 ]; do
    case "$1" in
    -nolowpri)      NOLOWPRI=1;;
    -highmem)       NOLOWPRI=1; HIGHMEM=1;;
    -nolocal)       NOLOCAL=1;;
    -quota)         arg_check 1 $# $1; QUOTA="$2"; shift;;
    -psub|-psub-opts|-psub-options)
                    arg_check 1 $# $1; PSUBOPTS="$PSUBOPTS $2"; shift;;
    -qsub|-qsub-opts|-qsub-options)
                    arg_check 1 $# $1; QSUBOPTS="$QSUBOPTS $2"; shift;;
    -v|-verbose)    VERBOSE=$(( $VERBOSE + 1 ));;
    -q|-quiet)      VERBOSE=0;;
    -d|-debug)      DEBUG=1;;
    -h|-help)       usage;;
    *)              break;;
    esac
    shift
done

test $# -eq 0   && error_exit "Missing commands file and <N> arguments"
test $# -eq 1   && error_exit "Missing <N> argument"

if which-test.sh qsub; then
    CLUSTER=1
else
    CLUSTER=
fi

CMD_FILE=$1; shift
NUM=$1;      shift

test X"$CMD_FILE" \!= X- -a \! -r "$CMD_FILE" &&
    error_exit "Can't read $CMD_FILE"
test $# -gt 0   && error_exit "Superfluous arguments $*"
test -z "$NUM"  && error_exit "Mandatory N argument is missing"
test $((NUM + 0)) != $NUM &&
    error_exit "Mandatory N argument must be numerical"
if [ -n "$QUOTA" ]; then
    test $((QUOTA + 0)) != $QUOTA &&
        error_exit "Value for -quota option must be numerical"
    QUOTA="-quota $QUOTA"
fi

if [ $VERBOSE -gt 1 ]; then
    echo "" >&2
    echo Starting run-parallel.sh \(pid $$\) on `hostname` on `date` >&2
    echo $0 $SAVE_ARGS >&2
    echo Using: >&2
    which faucet_launcher.sh faucet.pl worker.pl psub >&2
    echo "" >&2
fi

if [ $DEBUG ]; then
    echo "
    NUM=$NUM
    NOLOWPRI=$NOLOWPRI
    HIGHMEM=$HIGHMEM
    PSUBOPTS=$PSUBOPTS
    QSUBOPTS=$QSUBOPTS
    VERBOSE=$VERBOSE
    DEBUG=$DEBUG
    CMD_FILE=$CMD_FILE
" >&2
fi

MY_HOST=`hostname`
TMP_FILE_NAME=`/usr/bin/uuidgen -t`.${PBS_JOBID-local};

# Enable job control
#set -m

# Process clean up on exit or kill
trap '
    if [ -n "$WORKER_JOBIDS" ]; then
        qdel `cat $WORKER_JOBIDS` >& /dev/null
    fi
    rm -f psub-dummy-output $TMP_FILE_NAME.*
    exit
' 0 2 3 13 14 15


# save instructions from STDIN into instruction set
JOBSET_FILENAME=$TMP_FILE_NAME.jobs
cat $CMD_FILE > $JOBSET_FILENAME
NUM_OF_INSTR=$(wc -l < $JOBSET_FILENAME)

if [ $NUM_OF_INSTR = 0 ]; then
   echo "No commands to execute!  So I guess I'm done..." >&2
   exit
fi

if [ $NUM_OF_INSTR -lt $NUM ]; then
   echo "Lowering number of CPUs (was $NUM) to number of instructions ($NUM_OF_INSTR)" >&2
   NUM=$NUM_OF_INSTR
fi

# Fall-back mode in case faucet and netcat are not installed
NO_FAUCET_OR_NETCAT=0
if ! which-test.sh faucet; then
   NO_FAUCET_OR_NETCAT=1
fi
if ! which-test.sh netcat; then
   NO_FAUCET_OR_NETCAT=1
fi

if [ $NO_FAUCET_OR_NETCAT = 1 ]; then
   # We don't have the full facilities, run in basic mode: all jobs in the
   # background.
   if [ $NUM -lt $NUM_OF_INSTR ]; then
      echo \
"Run-parallel warning:  faucet or netcat not found - running in basic mode.
        Install faucet and netcat for optimal performance.
" >&2
   fi

   perl -e '
      my $num = shift;
      my @cmds = <>; chomp @cmds;
      my $script = "";
      foreach my $i (0 .. $#cmds) {
         $script .= "wait\n" if ($i > 0 && $i % $num == 0);
         $script .= "($cmds[$i]) &\n";
      }
      $script .= "wait";
      print $script, "\n";
      system($script);
   ' $NUM $JOBSET_FILENAME >&2
   exit
fi


if [ $VERBOSE -gt 1 ]; then
    faucet_launcher.sh $TMP_FILE_NAME &
    FAUCET_PROCESS=$!
elif [ $VERBOSE -gt 0 ]; then
    faucet_launcher.sh $TMP_FILE_NAME 2>&1 | grep '\] starting (' 1>&2 &
    FAUCET_PROCESS=$!
else
    faucet_launcher.sh $TMP_FILE_NAME >& /dev/null &
    FAUCET_PROCESS=$!
fi

# make sure we have a server listening, by sending a ping
while true; do
    sleep 1
    MY_PORT=`cat $TMP_FILE_NAME.port`
    if [ $VERBOSE -gt 1 ]; then
        echo Pinging $MY_HOST:$MY_PORT >&2
    fi
    if [ "`echo PING | netcat $MY_HOST $MY_PORT`" == "PONG" ]; then
        # faucet responded correctly, we're good to go now.
        break
    fi
done

if [ $VERBOSE -gt 1 ]; then
    echo faucet launched successfully on $MY_HOST:$MY_PORT >&2
fi

if [ ! $NOLOWPRI ]; then
    # By default specify ckpt=1, which means the job can run on borrowed nodes
    # Do this only on venus
    if pbsnodes -a 2> /dev/null | grep -q vns ; then
        PSUBOPTS="$PSUBOPTS -l ckpt=1"
    fi
fi

if [ $HIGHMEM ]; then
    # For high memory, request two CPUs per worker with ncpus=2.
    PSUBOPTS="$PSUBOPTS -2"
fi


# The psub command is fairly complex, so here it is documented in
# details
#
# Elements specified through SUBMIT_CMD:
#
#  - -e psub-dummy-output -o psub-dummy-output: overrides defaut .o and .e
#    output files generated by PBS/qsub, since we explicitely redirect
#    STDERR and STDOUT using > and 2>
#  - -noscript: don't save the .j script file generated by psub
#  - $PSUBOPTS: pass on any user-specified psub options
#  - qsparams "$QSUBOPTS": pass on any user-specified qsub options, as well as
#    a few added above by this script
#  - -v PERL5LIB: tell qsub to propagate the PERL5LIB environment
#    variable, on top of the default ones that always get propagated.
#
# Elements specified in the body of the for loop below:
#
#  - -N c-$i-$PBS_JOBID gives each sub job an easily interpretable
#    name
#  - \> $OUT 2\> $ERR (notice the \ before >) sends canoe's STDOUT
#    and STDERR to $OUT and $ERR, respectively
#  - >&2 (not escaped) sends psub's output to STDERR of this script.

SUBMIT_CMD=(psub -e psub-dummy-output -o psub-dummy-output
            -noscript
            $PSUBOPTS
            -qsparams "-v PERL5LIB $QSUBOPTS")

if [ $NOLOCAL ]; then
    FIRST_PSUB=0
else
    FIRST_PSUB=1
fi

# This file will contain the PBS job IDs of each worker
WORKER_JOBIDS=$TMP_FILE_NAME.worker_jobids
cat /dev/null > $WORKER_JOBIDS

# Command for launching more workers when some send a STOPPING-DONE message.
PSUB_CMD_FILE=$TMP_FILE_NAME.psub_cmd
if [ $CLUSTER ]; then
    cat /dev/null > $PSUB_CMD_FILE
    for word in "${SUBMIT_CMD[@]}"; do
        if echo "$word" | grep -q ' '; then
            echo -n "" \"$word\" >> $PSUB_CMD_FILE
        else
            # The "" must come before $word, otherwise -e is interpreted as an opt.
            echo -n "" $word >> $PSUB_CMD_FILE
        fi
    done
    echo -n "" -N w-__WORKER__ID__-${PBS_JOBID%%.*} "" >> $PSUB_CMD_FILE
    echo -n worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \\\> $TMP_FILE_NAME.out.worker-__WORKER__ID__ 2\\\> $TMP_FILE_NAME.err.worker-__WORKER__ID__ \>\> $WORKER_JOBIDS >> $PSUB_CMD_FILE
else
    echo worker.pl -host=$MY_HOST -port=$MY_PORT \> $TMP_FILE_NAME.out.worker-__WORKER__ID__ 2\> $TMP_FILE_NAME.err.worker-__WORKER__ID__ \& > $PSUB_CMD_FILE
fi
echo $NUM > $TMP_FILE_NAME.next_worker_id

# Provide (almost) cut and paste commands for doing quenches or adds
if [ $VERBOSE -gt 1 ]; then
   echo "To add N workers, do \"echo N > $TMP_FILE_NAME.add\"" >&2
   echo "To stop N workers, do \"echo N > $TMP_FILE_NAME.quench\"" >&2
fi

# start worker 0 locally, if not disabled.
if [ ! $NOLOCAL ]; then
    # start first worker locally (hostname of faucet process, number of
    # initial jobs in current call parameter n)
    OUT=$TMP_FILE_NAME.out.worker-0
    ERR=$TMP_FILE_NAME.err.worker-0
    if [ $VERBOSE -gt 2 ]; then
        echo worker.pl -host=$MY_HOST -port=$MY_PORT -primary \> $OUT 2\> $ERR \& >&2
    fi
    worker.pl -host=$MY_HOST -port=$MY_PORT -primary > $OUT 2> $ERR &
fi

# start workers 0 (or 1) to n-1 using psub, noting their PID
for (( i = $FIRST_PSUB ; i < $NUM ; ++i )); do
    # These should not end up being used by the commands, only by the
    # worker scripts themselves
    OUT=$TMP_FILE_NAME.out.worker-$i
    ERR=$TMP_FILE_NAME.err.worker-$i

    if [ $CLUSTER ]; then
        if [ $VERBOSE -gt 2 ]; then
            echo ${SUBMIT_CMD[@]} -N w-$i-${PBS_JOBID%%.*} worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \> $OUT 2\> $ERR >&2
        fi
        "${SUBMIT_CMD[@]}" -N w-$i-${PBS_JOBID%%.*} worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \> $OUT 2\> $ERR >> $WORKER_JOBIDS
        # PBS doesn't like having too many qsubs at once, let's give it a chance
        # to breathe between each worker submission
        sleep 1
    else
        if [ $VERBOSE -gt 2 ]; then
            echo worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \> $OUT 2\> $ERR \& >&2
        fi
        worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA > $OUT 2> $ERR &
    fi
done

if [ $CLUSTER ]; then
    # wait on faucet pid (faucet.pl will send SIGHUP to its parent, faucet,
    # when the last worker reports the last task is done)
    wait $FAUCET_PROCESS

    # Give PBS time to finish cleaning up worker jobs that have just finished
    sleep 5

    # kill all remaining psubed workers (which may not have been launched yet)
    # to clean things up.  (Ignore errors)
    qdel `cat $WORKER_JOBIDS` >& /dev/null
    WORKER_JOBIDS="/dev/null"
else
    # In non-cluster mode, just wait after everything, it gives us the exact
    # time when things are completely done.
    wait
fi

if [ $VERBOSE -gt 1 ]; then
    # Send all worker STDOUT and STDERR to STDERR for logging purposes.
    for (( i = 0; i < $NUM ; ++i )); do
        if [ -s $TMP_FILE_NAME.out.worker-$i ]; then
            echo Start worker $i STDOUT ===== >&2
            cat $TMP_FILE_NAME.out.worker-$i >&2
            echo End worker $i STDOUT ===== >&2
        fi
        if [ -s $TMP_FILE_NAME.err.worker-$i ]; then
            echo Start worker $i STDERR ===== >&2
            cat $TMP_FILE_NAME.err.worker-$i >&2
            echo End worker $i STDERR ===== >&2
        fi
    done
fi

if [ $VERBOSE -gt 1 ]; then
    echo "" >&2
    echo Done run-parallel.sh \(pid $$\) on `hostname` on `date` >&2
    echo $0 $SAVE_ARGS >&2
    echo "" >&2
fi


