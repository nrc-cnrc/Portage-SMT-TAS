#!/bin/bash
# $Id$
#
# monitor-process.sh : Watch a process using ps
#
# Eric Joanis
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

echo 'monitor-process.sh, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada' >&2

usage() {
    for msg in "$@"; do
        echo $msg >&2
    done
    cat <<==EOF== >&2

Usage: monitor-process.sh [-h(elp)] [-v(erbose)] <process_name>

  Periodically monitor any process named <process_name>.

Options:

  -p(eriod) <p>:  run every <p> seconds [5]
  -h(elp):        print this help message
  -v(erbose):     increment the verbosity level by 1 (may be repeated)
  -d(ebug):       print debugging information

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

# Command line processing [Remove irrelevant parts of this code when you use this template]
PERIOD=5
DEBUG=
VERBOSE=0
while [ $# -gt 0 ]; do
    case "$1" in
    -p|-period)     arg_check 1 $# $1; PERIOD=$2; shift;;
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
    PROCESS_NAME=$PROCESS_NAME
    DEBUG=$DEBUG
    VERBOSE=$VERBOSE
" >&2
fi

while true; do
    ps ugxf | grep $PROCESS_NAME | grep -v "grep.*$PROCESS_NAME" | grep -v monitor
    date
    sleep $PERIOD 
done
