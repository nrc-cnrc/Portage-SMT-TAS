#!/bin/bash
# $Id$

# @file prog.sh 
# @brief Briefly describe your script here.
# 
# @author Write your name here
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

echo 'prog.sh, NRC-CNRC, (c) 2009, Her Majesty in Right of Canada' >&2

# Includes NRC's bash library.
source `dirname $0`/sh_utils.sh

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: $0 PROG_ARGUMENT1 PROG_ARGUMENT2

  Brief description

Options:

  -flag       turn on <insert behaviour>
  -option-with-1arg ARG         ...
  -option-with-2args ARG1 ARG2  ...
  -int-opt INT                  ...
  -pos-int-opt POS_INT          ...
  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -flag)               FLAG=1;;
   -option-with-1arg)   arg_check 1 $# $1; OPTION_WITH_1ARG_VALUE=$2; shift;;
   -option-with-2args)  arg_check 2 $# $1
                        OPTION_WITH_2ARGS_ARG1=$2;
                        OPTION_WITH_2ARGS_ARG2=$3;
                        shift; shift;;
   -int-opt)            arg_check 1 $# $1; arg_check_int $2 $1
                        INT_ARG=$2; shift;;
   -pos-int-opt)        arg_check 1 $# $1; arg_check_pos_int $2 $1
                        POS_INT_ARG=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0   && error_exit "Missing first argument argument"
PROG_ARGUMENT1=$1; shift
test $# -eq 0   && error_exit "Missing second argument argument"
PROG_ARGUMENT2=$1; shift
test $# -gt 0   && error_exit "Superfluous arguments $*"

# [Update this block with your variables, or simply delete it, when you use
# this template]
if [ $DEBUG ]; then
   echo "
   FLAG=$FLAG
   OPTION_WITH_1ARG_VALUE=$OPTION_WITH_1ARG_VALUE
   OPTION_WITH_2ARGS_ARG1=$OPTION_WITH_2ARGS_ARG1
   OPTION_WITH_2ARGS_ARG2=$OPTION_WITH_2ARGS_ARG2
   INT_ARG=$INT_ARG
   POS_INT_ARG=$POS_INT_ARG
   DEBUG=$DEBUG
   VERBOSE=$VERBOSE
   REMAINING ARGS=$*
" >&2
fi

[[ $VERBOSE -gt 0 ]] && echo mildly verbose output

[[ $VERBOSE -gt 1 ]] && echo very verbose output

[[ $VERBOSE -gt 2 ]] && echo exceedingly verbose output

