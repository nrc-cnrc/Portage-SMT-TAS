#!/bin/bash
# $Id$
#
# @file monitor-process.sh 
# @brief Watch a process using ps.
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

# Include NRC's bash library.
source `dirname $0`/sh_utils.sh

[[ $PORTAGE_INTERNAL_CALL ]] ||
print_nrc_copyright monitor-process.sh 2005
export PORTAGE_INTERNAL_CALL=1

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: monitor-process.sh [-h(elp)] [-v(erbose)] <process_name>

  Periodically monitor any process named <process_name>.

Options:

  -s <#>:         stop if the process is not present after # time. [infinite]
  -p(eriod) <p>:  run every <p> seconds [5]
  -p10 <p>        same as -p, but specified in tenths of a second
  -h(elp):        print this help message
  -v(erbose):     increment the verbosity level by 1 (may be repeated)
  -d(ebug):       print debugging information

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use this template]
PERIOD=5
DEBUG=
VERBOSE=0
STOP_AFTER=
while [ $# -gt 0 ]; do
   case "$1" in
   -s)             arg_check 1 $# $1; STOP_AFTER=$2; shift;;
   -p|-period)     arg_check 1 $# $1; PERIOD=$2; shift;;
   -p10)           arg_check 1 $# $1; PERIOD_TENTH=$(($2 * 100000)); shift;;
   -v|-verbose)    VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)      DEBUG=1;;
   -h|-help)       usage;;
   --)             shift; break;;
   -*)             error_exit "Unknown option $1.";;
   *)              break;;
   esac
   shift
done

if [ $# -ne 1 ]; then
   error_exit "Missing process name or superfluous arguments."
fi

PROCESS_NAME=$1

# [Update this block with your variables, or simply delete it, when you use
# this template]
if [ $DEBUG ]; then
   echo "
   PERIOD=$PERIOD
   PERIOD_TENTH=$PERIOD_TENTH
   PROCESS_NAME=$PROCESS_NAME
   DEBUG=$DEBUG
   VERBOSE=$VERBOSE
" >&2
fi

not_running=0
while true; do
   ps ugxf | grep $PROCESS_NAME | grep -v "grep.*$PROCESS_NAME" | grep -v monitor
   RUNNING=$?
   if [ -n "$STOP_AFTER" ]; then
      echo "Running: $RUNNING"
      if [ $RUNNING -gt 0 ]; then 
         not_running=$((not_running + 1))
         [ $not_running -gt $STOP_AFTER ] && exit 0;
      fi
   fi
   date
   if [ $PERIOD_TENTH ]; then
      usleep $PERIOD_TENTH
   else
      sleep $PERIOD 
   fi
done
