#!/bin/bash
# vim:noet:ts=3:nowrap
# $Id$
# @file prep-file-layoyt.sh
# @brief Fetch trained Portage models and create the proper layout for
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

Usage: prep-file-layout.sh <source> [<context_label>]

  Fetch trained models from <source>, which has to be the directory where
  a Portage system was trained using the experimental framework.
  The files are placed in the proper structure for building an RPM to install
  on a translation server.

  <context_label> is the label of the context: models are prepared for
  installation under /opt/Portage/models/<context_label>.  ["context"]

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

shopt -s compat31 >& /dev/null
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

if [[ $# -gt 0 ]]; then
   CONTEXT=$1;
   shift;
else
   CONTEXT=context
fi

[[ $# -gt 0 ]] && usage "Superfluous argument(s) $*"

DESTINATION=rpm.build.root/opt/Portage/models/$CONTEXT
mkdir -p $DESTINATION
if [[ $SOURCE =~ : ]]; then
   CP_CMD="scp -r"
else
   CP_CMD="cp -Lr"
fi
$CP_CMD $SOURCE/models/portageLive/* $DESTINATION/

# Let's add a md5sum since building a rpm for the models is error prone.
pushd rpm.build.root/opt/Portage/models/$CONTEXT && find -type f | egrep -v md5 | xargs md5sum > md5 && popd

# Set proper permissions on the directory and file structure
find rpm.build.root -type d | xargs chmod 755
find rpm.build.root -type f | xargs chmod 644
find -type f -name \*.sh | xargs chmod 755
if [[ -d $DESTINATION/plugins ]]; then
   [[ `find $DESTINATION/plugins -type f` ]] && chmod 755 $DESTINATION/plugins/*
fi

exit

