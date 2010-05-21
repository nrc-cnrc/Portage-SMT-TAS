#!/bin/bash
# vim:noet:ts=3:nowrap
# $Id$
# @file prep-file-layoyt.sh
# @brief Fetch a set Portage system aka models and create the proper layout for
#        portageLive.
# 
# @author Samuel Larkin
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2

Usage: prep-file-layout.sh <source>

  Fetch from <source>, a tuned portage simple framework system, all models that
  are needed for portageLive and creates the proper layout for the rpm. 

==EOF==

   exit 1
}

arg_check() {
   if [ $2 -le $1 ]; then
      usage "Missing argument to $3 option."
   fi
}

# error_exit "some error message" "optionnally a second line of error message"
# will exit with an error status, print the specified error message(s) on
# STDERR.
error_exit() {
   {
      PROG_NAME=`basename $0`
      echo -n "$PROG_NAME fatal error: "
      for msg in "$@"; do
         echo $msg
      done
      echo "Use -h for help."
   } >&2
   exit 1
}

set -o errexit

while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0  && error_exit "Missing source directory argument"
SOURCE=$1; shift
[[ $# -gt 0 ]] && usage "Superfluous argument(s) $*"

DESTINATION=rpm.build.root/opt/Portage/models/context
mkdir -p $DESTINATION
scp -r $SOURCE/models/portageLive/* $DESTINATION/

# Set proper permissions on the directory and file structure
find rpm.build.root -type d | xargs chmod 755
find rpm.build.root -type f | xargs chmod 644
find -type f -name \*.sh | xargs chmod 755
[[ `find $DESTINATION/plugins -type f` ]] && chmod 755 $DESTINATION/plugins/*

exit

