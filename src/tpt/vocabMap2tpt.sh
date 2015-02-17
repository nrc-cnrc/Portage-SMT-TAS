#!/bin/bash

# $Id$

# @file vocabMap2tpt.sh
# @brief Transform a vocabulary map into a tpt.
# 
# @author Samuel Larkin
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

echo 'vocabMap2tpt.sh, (c) 2005-2010, Ulrich Germann and Her Majesty in Right of Canada' >&2

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

# Exit if a pipe fails.
set -o pipefail;
# Exit if a command return non-zero.
set -e;

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: vocabMap2tpt.sh <VocabMap> [TPPT_prefix]

  Transform a vocabulary map into a tpt.

Options:

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

# Command line processing [Remove irrelevant parts of this code when you use
# this template]
DEBUG=
VERBOSE=0
while [ $# -gt 0 ]; do
   case "$1" in
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0   && error_exit "Missing vocabulary map model."
VocabMapFile=$1; shift
if [[ $1 ]]; then
   TPPT_prefix=$1; shift
   if [[ `basename $TPPT_prefix` != $TPPT_prefix ]]; then
      error_exit "Output model must be created in current directory, without path specification."
   fi
else
   TPPT_prefix=`basename $VocabMapFile .gz`
fi
test $# -gt 0   && error_exit "Superfluous arguments $*"


# We need a intermediate model to hold the textpt.
tmpModel=`mktemp vocabMap2tpt.XXX` || error_exit "Cannot create temp file."
# Set automatic clean up unless we are debugging.
test -n $debug || trap "rm -f $tmpModel" 0 2 3 5 10 13 15
verbose 2 "Intermediate model is $tmpModel"


# Creating a textpt from a vocabulary map.
verbose 1 "Creating intermediate textpt model ($tmpModel)."
zcat -f $VocabMapFile \
| perl -nle ' 
   BEGIN{binmode(STDIN); binmode(STDOUT)}
   my @line = split( /\t/, $_ ); 
   my $from = shift( @line ); 
   my $to   = shift( @line ); 
   my $prob = shift( @line ); 
   while (    defined( $to )   && $to ne "" 
           && defined( $prob ) && $prob ne "" ) { 
      print "$from ||| $to ||| 1 $prob"; 
      $to   = shift( @line ); 
      $prob = shift( @line ); 
   } ' \
> $tmpModel


# Converting the intermediate textpt to a tpt.
verbose 1 "Creating the tpt model."
textpt2tppt.sh $tmpModel $TPPT_prefix

