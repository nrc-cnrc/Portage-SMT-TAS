#!/bin/bash
# $Id$

# @file stub-feature-function.sh 
# @brief This is a stub to be able to test the new script feature function in rat.sh.
# 
# @author Samuel Larkin
# 
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada


usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: stub-feature-function.sh [-h|<nbest.file.gz>] ....

  Stub to test the script feature function in rat.sh.
  Requires an nbest.
  Allows more than 1 option all others are ignored (good for debugging tags in
  rat.sh).

  Options:
  -h for help
  mandatory nbest list file

==EOF==

    exit 1
}

test "$1" == "-h" && usage;

zcat -f $1 | perl -nle "print 2"
