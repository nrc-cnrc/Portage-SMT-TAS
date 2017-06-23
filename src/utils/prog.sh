#!/bin/bash

# @file prog.sh 
# @brief Briefly describe your script here.
# 
# @author Write your name here
# 
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG PROG_ARGUMENT1 PROG_ARGUMENT2

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

[[ $# -eq 0 ]]  && error_exit "Missing first argument argument"
PROG_ARGUMENT1=$1; shift
[[ $# -eq 0 ]]  && error_exit "Missing second argument argument"
PROG_ARGUMENT2=$1; shift
[[ $# -gt 0 ]]  && error_exit "Superfluous arguments $*"

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

