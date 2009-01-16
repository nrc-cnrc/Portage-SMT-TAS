#!/bin/bash
# $Id$

# @file run-after.sh 
# @brief Run a command after a given job has terminated.
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

echo 'run-after.sh, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada' >&2

usage() {
    for msg in "$@"; do
        echo $msg >&2
    done
    cat <<==EOF== >&2

Usage: run-after.sh <pid> <command line>

E.g.:  run-after.sh 12345 wrapper.sh
       run-after.sh 12345 ls \"file name with spaces\" \> output-file

Notes:
 - Escape any quotes and any pipe or redirection symbols used.
 - Can be daisy-chained: use the pid of the previous run-after.sh job itself.
 - Will not do anything if job <pid> is not running.
 - If <pid> 0, runs the command line immediately. (Useful for debugging.)

==EOF==

    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
    -h|-help)       usage;;
    *)              break;;
    esac
    shift
done

if [ $# -lt 2 ]; then
    echo $# $*
    usage "missing pid or command"
fi

AFTER_PID=$1; shift
COMMAND=$*


if [ $AFTER_PID == 0 ]; then
    echo Job PID specified as 0, immediately running "$COMMAND" >&2
    eval $COMMAND
    exit $?
elif ps $AFTER_PID > /dev/null 2>&1; then
    echo Job $AFTER_PID is running.  When it stops, I will run "$COMMAND" >&2
else
    echo Job $AFTER_PID is not running, are you sure you got the right PID? >&2
    echo Not doing anything. >&2
    usage
fi

while ps $AFTER_PID > /dev/null; do
    sleep 60
done

echo Job $AFTER_PID finished, running "$COMMAND" >&2

eval $COMMAND
exit $?
