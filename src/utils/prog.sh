#!/bin/bash
# $Id$

# prog.sh - bash script template [Update this line when you use this template]
# 
# PROGRAMMER: Eric Joanis [Update this line when you use this template]
# 
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

echo 'prog.sh, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada' >&2

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: prog.sh [-h(elp)] [-v(erbose)] [-flag] [-option_with_1arg <arg>]
       [-option_with_2_args <arg1> <arg2>] prog_arguments

  Brief description

Options:

  -flag:        turn on <insert behaviour>

  -option_with_1arg <arg>: ...

  -option_with_2args <arg1> <arg2>: ...

  -h(elp):      print this help message

  -v(erbose):   increment the verbosity level by 1 (may be repeated)

  -d(ebug):     print debugging information

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
FLAG=
OPTION_WITH_1ARG_VALUE=
OPTION_WITH_2ARGS_ARG1=
OPTION_WITH_2ARGS_ARG2=
DEBUG=
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -flag)               FLAG=1;;
   -option_with_1arg)   arg_check 1 $# $1; OPTION_WITH_1ARG_VALUE=$2; shift;;
   -option_with_2args)  arg_check 2 $# $1
                        OPTION_WITH_2ARGS_ARG1=$2;
                        OPTION_WITH_2ARGS_ARG2=$3;
                        shift; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

# [Update this block with your variables, or simply delete it, when you use
# this template]
if [ $DEBUG ]; then
   echo "
   FLAG=$FLAG
   OPTION_WITH_1ARG_VALUE=$OPTION_WITH_1ARG_VALUE
   OPTION_WITH_2ARGS_ARG1=$OPTION_WITH_2ARGS_ARG1
   OPTION_WITH_2ARGS_ARG2=$OPTION_WITH_2ARGS_ARG2
   DEBUG=$DEBUG
   VERBOSE=$VERBOSE
   REMAINING ARGS=$*
" >&2
fi

if [ $VERBOSE -gt 0 ]; then
   echo mildly verbose output
fi

if [ $VERBOSE -gt 1 ]; then
   echo very verbose output
fi

if [ $VERBOSE -gt 2 ]; then
   echo exceedingly verbose output
fi
