#!/bin/bash

# @file process-bind.sh
# @brief Bind a process to a master process, killing the bound process when
#        the master stops running.
# 
# @author Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

# Include NRC's bash library.
source `dirname $0`/sh_utils.sh

print_nrc_copyright process-bind.sh 2009

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG -pid PID [options] COMMAND ARGUMENTS

  Run "COMMAND ARGUMENTS", killing it if master process PID disappears.

Options:

  -interval   check every SECONDS seconds whether PID is still running [60]
  -pid        (mandatory) specifies the master process to watch
  -h(elp)     print this help message
  -q(uiet)    suppress verbose messages
  -d(ebug)    print additional debugging information

Exit status:

  If COMMAND finishes before PID disappears, the exit status is that of
  COMMAND itself.
  If process PID disappears and COMMAND is killed, the exit status is 45.
  If there is an error before running COMMAND, the exit status is 1.

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
INTERVAL=30
VERBOSE=1
while [ $# -gt 0 ]; do
   case "$1" in
   -pid)                arg_check 1 $# $1; arg_check_pos_int $2 $1
                        PID=$2; shift;;
   -interval)           arg_check 1 $# $1; arg_check_pos_int $2 $1
                        INTERVAL=$2; shift;;
   -v|-verbose)         VERBOSE=1;;
   -q|-quiet)           VERBOSE=;;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $PID || error_exit "Missing -pid specification"
if ps -p $PID >& /dev/null; then
   test $VERBOSE &&
      echo "Launching command: $*" >&2
      echo "Will kill it if master process ($PID) stops." >&2
else
   error_exit "Process $PID is not running; not launching command." >&2
fi

trap "kill %1; kill -1 %2; echo Got killed. Terminating child process. >&2; exit -1" 2 3 15

watch_master() {
   while true; do
      sleep $INTERVAL
      if ps -p $PID >& /dev/null; then
         test $DEBUG &&
            echo "Process $PID is still running; continuing." >&2
      else
         echo "Process $PID is no longer running; stopping." >&2
         kill $CHILD_PID
         exit 45
      fi
   done
}

set -o monitor
#echo "$@"
"$@" &
CHILD_PID=$!

watch_master &
WATCH_PID=$!

test $DEBUG &&
   echo "Child PID = $PID." >&2
test $DEBUG &&
   echo "Watch PID = $WATCH_PID." >&2

wait $CHILD_PID
RC=$?

if ps -p $WATCH_PID >& /dev/null; then
   kill -1 %2
fi

exit $RC

