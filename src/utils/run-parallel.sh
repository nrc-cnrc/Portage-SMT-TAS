#!/bin/bash
# $Id$

# run-parallel.sh - runs a series of jobs provided as STDIN on parallel
#                   distributed workers managed by faucet.pl / worker.pl.
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
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
       run-parallel.sh [options] -e <cmd1> [-e <cmd2> ...] <N>
       run-parallel.sh add <M> <JOB_ID>
       run-parallel.sh quench <M> <JOB_ID>

  Execute commands on <N> parallel workers, each submitted with psub on a
  cluster, or run as background tasks otherwise.  Except in -basic mode,
  keeps the CPUs constantly busy until all jobs are completed.

Exit status:
  0: all OK
  1: usage error
  2: at least one job had a non-zero exit status
  -1/255: something strange happened, probably a crash of some kind

Arguments:

  <file_of_commands>  A file with exactly one command per line, in valid
      bash syntax.  (Use - for STDIN.)  Each command may include redirections
      and pipes, will run in the current directory, and will be aware of the
      current value of PATH.  Commands will run in an arbitrary order.  Each
      command should explicitly redirect its output, or risk losing it.

  <cmd#> A command, properly quoted.  Specifying -e once or more is equivalent
      to specifying a <file_of_commands> with one or more lines.  This option
      is provided so that run-parallel.sh can be used as a "blocking" psub.
          run-parallel.sh -nolocal -e <cmd>
      is equivalent to "psub <cmd>" except that, unlike psub, it only returns
      when <cmd> has finished running, and the exit status of run-parallel.sh
      will be 0 iff the exit status of <cmd> was 0.

  <N> Number of workers to launch (may differ from the number of commands).

General options:

  -h(elp)       Print this help message.
  -d(ebug)      Print debugging information.
  -q(uiet)      Quiet mode only prints commands executed.

Cluster mode options:

  -highmem      Use 2 CPUs per worker, for extra extra memory.  [default is
                to propagate the number of CPUs requested by master job]
  -nohighmem    Use only 1 CPU per worker, even if master job had more.
  -nolocal      psub all workers (by default, one worker is run locally)
  -nocluster    force non-cluster mode
  -quota T      When workers have done T minutes of work, re-psub them [60]
  -psub         Provide custom psub options.
  -qsub         Provide custom qsub options.
  -basic        force basic mode (without socket/deamon); implies -nocluster.
                runs jobs in the background locally - for fast jobs (lasting a
                few seconds or less), this will be faster than the regular
                mode, which incurs an overhead of a few seconds for each job.

Dynamic options:

  To add M new workers on the fly, identify the PBS job id of the master, or
  the <long_prefix>.psub_cmd file where it is running, and run:
     run-parallel.sh add M <PBS job id>
  or
     run-parallel.sh add M <long_prefix>.psub_cmd

  To quench the process and remove M workers, replace "add" by "quench".

  To kill the process and all its workers, replace "add M" by "kill".

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
HIGHMEM=
NOHIGHMEM=
NOLOCAL=
NOCLUSTER=
VERBOSE=1
DEBUG=
SAVE_ARGS="$@"
CMD_FILE=
CMD_LIST=
QUOTA=
MY_HOST=`hostname`
TMP_FILE_NAME=`/usr/bin/uuidgen -t`.${PBS_JOBID-local};
JOBSET_FILENAME=$TMP_FILE_NAME.jobs
while [ $# -gt 0 ]; do
   case "$1" in
   -e)             CMD_LIST="$CMD_LIST:$2"; echo "$2">>$JOBSET_FILENAME
                   shift;;
   -highmem)       HIGHMEM=1;;
   -nohighmem)     NOHIGHMEM=1;;
   -nolocal)       NOLOCAL=1;;
   -nocluster)     NOCLUSTER=1;;
   -basic)         BASIC_MODE=1;;
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

if [ "$1" = add -o "$1" = quench -o "$1" = kill ]; then
   # Special command to dynamically add or remove worders to/from a job in
   # progress
   if [ "$1" = kill ]; then
      if [ $# != 2 ]; then
         error_exit "Kill requests take exactly 2 parameters."
      fi
      REQUEST_TYPE=$1
      JOB_ID_OR_CMD_FILE=$2
   else
      if [ $# != 3 ]; then
         error_exit "Dynamic add and quench requests take exactly 3 parameters."
      fi
      REQUEST_TYPE=$1
      NUM=$2
      if [ "`expr $NUM + 0 2> /dev/null`" != "$NUM" ]; then
         error_exit "$NUM is not an integer."
      fi
      if [ $NUM -lt 1 ]; then
         error_exit "$NUM is not a positive integer."
      fi
      JOB_ID_OR_CMD_FILE=$3
   fi
   if [ -f $JOB_ID_OR_CMD_FILE ]; then
      CMD_FILE=$JOB_ID_OR_CMD_FILE
   else
      JOB_ID=$JOB_ID_OR_CMD_FILE
      JOB_PATH=`qstat -f $JOB_ID | perl -e '
         undef $/;
         $_ = <>;
         s/\s//g;
         my ($path) = (/PBS_O_WORKDIR=(.*?),/);
         print $path;
      '`
      echo Job ID: $JOB_ID
      echo Job Path: $JOB_PATH
      CMD_FILE=`\ls $JOB_PATH/*.$JOB_ID*.psub_cmd 2> /dev/null`
      if [ ! -f "$CMD_FILE" ]; then
         error_exit "Can't find command file for job $JOB_ID"
      fi
   fi

   if [ $DEBUG ]; then
      echo Psub Cmd File: $CMD_FILE
   fi
   HOST=`perl -e 'undef $/; $_ = <>; ($host) = /-host=(.*?) /; print $host' < $CMD_FILE`
   PORT=`perl -e 'undef $/; $_ = <>; ($port) = /-port=(.*?) /; print $port' < $CMD_FILE`
   echo Host: $HOST:$PORT

   if [ $REQUEST_TYPE = add ]; then
      RESPONSE=`echo ADD $NUM | netcat "$HOST" "$PORT"`
      if [ "$RESPONSE" != ADDED ]; then
         error_exit "Faucet error (response=$RESPONSE), add request failed."
      fi
      if [ "`echo PING | netcat \"$HOST\" \"$PORT\"`" != "PONG" ]; then
         echo "Faucet does not appear to be running, request completed" \
              "but likely won't work."
         exit 1
      fi
   elif [ $REQUEST_TYPE = quench ]; then
      RESPONSE=`echo QUENCH $NUM | netcat "$HOST" "$PORT"`
      if [ "$RESPONSE" != QUENCHED ]; then
         error_exit "Faucet error (response=$RESPONSE), quench request failed."
      fi
   elif [ $REQUEST_TYPE = kill ]; then
      RESPONSE=`echo KILL | netcat "$HOST" "$PORT"`
      if [ "$RESPONSE" != KILLED ]; then
         error_exit "Faucet error (response=$RESPONSE), kill request failed."
      fi
      echo "Killing faucet and all workers (will take several seconds)."
      exit 0
   else
      error_exit "Internal script error - invalid request type: $REQUEST_TYPE."
   fi

   echo Dynamically ${REQUEST_TYPE}ing $NUM 'worker(s)'

   exit 0
fi

if which-test.sh qsub; then
   CLUSTER=1
else
   CLUSTER=
fi
if [ $NOCLUSTER ]; then
   CLUSTER=
fi

# save instructions from STDIN into instruction set
if [ -n "$CMD_LIST" ]; then
   test $# -eq 0   && error_exit "Missing <N> argument"
   NUM=$1;      shift
   test -n "$CMD_FILE" && error_exit "Can't use a file when using -e option $CMD_FILE as a file"
else
   test $# -eq 0   && error_exit "Missing commands file and <N> arguments"
   test $# -eq 1   && error_exit "Missing <N> argument"

   CMD_FILE=$1; shift
   NUM=$1;      shift

   test X"$CMD_FILE" \!= X- -a \! -r "$CMD_FILE" &&
      error_exit "Can't read $CMD_FILE"
   cat $CMD_FILE > $JOBSET_FILENAME
fi
NUM_OF_INSTR=$(wc -l < $JOBSET_FILENAME)


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
   which run-parallel-d.pl worker.pl psub >&2
   echo "" >&2
fi

if [ $DEBUG ]; then
   echo "
   NUM=$NUM
   HIGHMEM=$HIGHMEM
   NOHIGHMEM=$NOHIGHMEM
   PSUBOPTS=$PSUBOPTS
   QSUBOPTS=$QSUBOPTS
   VERBOSE=$VERBOSE
   DEBUG=$DEBUG
   CMD_FILE=$CMD_FILE
   CMD_LIST=$CMD_LIST
" >&2
fi

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


if [ $NUM_OF_INSTR = 0 ]; then
   echo "No commands to execute!  So I guess I'm done..." >&2
   exit
fi

if [ $NUM_OF_INSTR -lt $NUM ]; then
   echo "Lowering number of CPUs (was $NUM) to number of instructions ($NUM_OF_INSTR)" >&2
   NUM=$NUM_OF_INSTR
fi

# Fall-back mode in case netcat is not installed
NO_NETCAT=0
if ! which-test.sh netcat; then
   NO_NETCAT=1
fi

if [ $NO_NETCAT = 1 -o -n "$BASIC_MODE" ]; then
   # We don't have the full facilities, run in basic mode: all jobs in the
   # background.
   if [ $NUM -lt $NUM_OF_INSTR -a -z "$BASIC_MODE" ]; then
      echo \
"Run-parallel warning:  netcat not found - running in basic mode.
        Install netcat for optimal performance.
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
   run-parallel-d.pl $TMP_FILE_NAME &
   DEAMON_PID=$!
elif [ $VERBOSE -gt 0 ]; then
   run-parallel-d.pl $TMP_FILE_NAME 2>&1 | egrep '\] ([0-9/]* DONE.*|starting) \(' 1>&2 &
   DEAMON_PID=$!
else
   run-parallel-d.pl $TMP_FILE_NAME >& /dev/null &
   DEAMON_PID=$!
fi

# make sure we have a server listening, by sending a ping
while true; do
   sleep 1
   MY_PORT=`cat $TMP_FILE_NAME.port`
   if [ $VERBOSE -gt 1 ]; then
      echo Pinging $MY_HOST:$MY_PORT >&2
   fi
   if [ "`echo PING | netcat $MY_HOST $MY_PORT`" = PONG ]; then
      # faucet responded correctly, we're good to go now.
      break
   fi
done

if [ $VERBOSE -gt 1 ]; then
   echo faucet launched successfully on $MY_HOST:$MY_PORT >&2
fi

if [ $HIGHMEM ]; then
   # For high memory, request two CPUs per worker with ncpus=2.
   PSUBOPTS="$PSUBOPTS -2"
elif [ -z "$nohighmem" -a -n "$pbs_jobid" ]; then
   if which qstat >& /dev/null; then
      if qstat -f $pbs_jobid 2> /dev/null | grep -q 1:ppn=2 >& /dev/null; then
         echo master was submitted with 2 cpus, propagating to workers >&2
         psubopts="$psubopts -2"
      elif qstat -f $pbs_jobid 2> /dev/null | grep -q 1:ppn=3 >& /dev/null; then
         echo master was submitted with 3 cpus, propagating to workers >&2
         psubopts="$psubopts -3"
      elif qstat -f $pbs_jobid 2> /dev/null | grep -q 1:ppn=4 >& /dev/null; then
         echo master was submitted with 4 cpus, propagating to workers >&2
         psubopts="$psubopts -4"
      fi
   fi
fi

if [ -n "$PBS_JOBID" ]; then
   MASTER_PRIORITY=`qstat -f $PBS_JOBID 2> /dev/null | egrep 'Priority = -?[0-9][0-9]*$' | sed 's/.*Priority = //'`
fi
if [ -z "$MASTER_PRIORITY" ]; then
   MASTER_PRIORITY=0
fi
#echo MASTER_PRIORITY $MASTER_PRIORITY
PSUBOPTS="$PSUBOPTS -p $((MASTER_PRIORITY - 1))"
#echo PSUBOPTS $PSUBOPTS


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
#
# Elements specified in the body of the for loop below:
#
#  - -N c-$i-$PBS_JOBID gives each sub job an easily interpretable
#    name
#  - \> $OUT 2\> $ERR (notice the \ before >) sends canoe's STDOUT
#    and STDERR to $OUT and $ERR, respectively
#  - >&2 (not escaped) sends psub's output to STDERR of this script.

SUBMIT_CMD=(psub
            -e psub-dummy-output
            -o psub-dummy-output
            -noscript
            $PSUBOPTS)

if [ -n "$QSUBOPTS" ]; then
   SUBMIT_CMD=("${SUBMIT_CMD[@]}" -qsparams "$QSUBOPTS")
fi

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
   echo -n "" -N w__WORKER__ID__-${PBS_JOBID%%.*} >> $PSUB_CMD_FILE
   echo -n "" worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \\\> $TMP_FILE_NAME.out.worker-__WORKER__ID__ 2\\\> $TMP_FILE_NAME.err.worker-__WORKER__ID__ \>\> $WORKER_JOBIDS >> $PSUB_CMD_FILE
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
         echo ${SUBMIT_CMD[@]} -N w$i-${PBS_JOBID%%.*} worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \> $OUT 2\> $ERR >&2
      fi
      "${SUBMIT_CMD[@]}" -N w$i-${PBS_JOBID%%.*} worker.pl -host=$MY_HOST -port=$MY_PORT $QUOTA \> $OUT 2\> $ERR >> $WORKER_JOBIDS
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
   # wait on deamon pid (run-parallel-d.pl, the deamon, will exit when the last
   # worker reports the last task is done)
   wait $DEAMON_PID

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

if [ $VERBOSE -gt 0 ]; then
   # show the exit status of each worker
   echo -n 'Exit status(es) from all jobs (in the order they finished): ' >&2
   cat $TMP_FILE_NAME.rc 2> /dev/null | tr '\n' ' ' >&2
   echo "" >&2
fi

if [ $VERBOSE -gt 1 ]; then
   # Send all worker STDOUT and STDERR to STDERR for logging purposes.
   for x in $TMP_FILE_NAME.{out,err}.worker-*; do
      if [ -s $x ]; then
         echo ========== $x ========== | sed "s/$TMP_FILE_NAME.//" >&2
         cat $x >&2
      fi
   done
fi

if [ $VERBOSE -gt 1 ]; then
   echo "" >&2
   echo Done run-parallel.sh \(pid $$\) on `hostname` on `date` >&2
   echo $0 $SAVE_ARGS >&2
   echo "" >&2
fi

if [ `wc -l < $TMP_FILE_NAME.rc` -ne "$NUM_OF_INSTR" ]; then
   echo 'Wrong number of job return statuses: got' `wc $TMP_FILE_NAME.rc` "expected $NUM_OF_INSTR." >&2
   exit -1
elif grep -q -v '^0$' $TMP_FILE_NAME.rc >& /dev/null; then
   # At least one job got a non-zero return code
   exit 2
else
   exit 0
fi
