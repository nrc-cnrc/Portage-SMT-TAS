#!/bin/bash
# $Id$

# make-distro.sh - Make a CD or a copyable distro of PORTAGEshared.
# 
# PROGRAMMER: Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

echo 'make-distro.sh, NRC-CNRC, (c) 2007, Her Majesty in Right of Canada'

usage() {
   for msg in "$@"; do
      echo $msg
   done
   cat <<==EOF==

Usage: make-distro.sh [-h(elp)] [-bin] [-nosrc] [-licence PROJECT]
       -tag CVS_TAG_OPTION -dir OUTPUT_DIR

  Make a PORTAGEshared distribution folder, ready to burn on CD or copy to a
  remote site as is.

Arguments:

  -tag          What source to use for this distro, as a well formatted CVS
                option.  CVS_TAG_OPTION should be of the form -rCVSTAG, with
                CVSTAG having been created first using "cvs tag -R vX_Y" on the
                whole PORTAGEshared repository.  It can also be any valid -r or
                -D option that can be given to "cvs co", if necessary.

  -dir          The distro will be created in OUTPUT_DIR, which must not
                already exist.

Options:

  -h(elp):      print this help message
  -bin:         include compiled code [don't, unless -nosrc is specified]
  -nosrc        do not include src code [do]
  -licence      Copy the LICENCE file for PROJECT.  Can be CanUniv, SMART,
                or BinOnly (which means no LICENCE* file is copied).
                [CanUniv, or BinOnly if -nosrc is specified]

==EOF==

    exit 1
}

error_exit() {
   for msg in "$@"; do
      echo $msg
   done
   echo "Use -h for help."
   exit 1
}

run_cmd() {
   echo "$*"
   eval $*
   rc=$?
   if [ "$rc" != 0 ]; then
      echo "Exit status: $rc is not zero - aborting."
      exit 1
   fi
}

arg_check() {
   if [ $2 -le $1 ]; then
      error_exit "Missing argument to $3 option."
   fi
}


while [ $# -gt 0 ]; do
   case "$1" in
   -bin)                INCLUDE_BIN=1;;
   -nosrc)              NO_SOURCE=1;;
   -tag)                arg_check 1 $# $1; VERSION_TAG=$2; shift;;
   -dir)                arg_check 1 $# $1; OUTPUT_DIR=$2; shift;;
   -licence|-license)   arg_check 1 $# $1; LICENCE=$2; shift;;
   -v|-verbose)         VERBOSE=$(( $VERBOSE + 1 ));;
   -debug)              DEBUG=1;;
   -h|-help)            usage;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   error_exit "Extraneous command line argument $1.";;
   esac
   shift
done


test $VERSION_TAG || error_exit "Missing mandatory -tag argument"
test $OUTPUT_DIR  || error_exit "Missing mandatory -dir argument"

skip() {

test -d $OUTPUT_DIR && error_exit "$OUTPUT_DIR exists - won't overwrite -delete it first."

run_cmd mkdir $OUTPUT_DIR
run_cmd cd $OUTPUT_DIR
run_cmd cvs co $VERSION_TAG PORTAGEshared
run_cmd find PORTAGEshared -name CVS \| xargs rm -rf

if [ "$LICENCE" = SMART ]; then
   echo Keeping only SMART licence info.
   run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| \
           grep -v -x PORTAGEshared/LICENCE_SMART \| xargs rm -f
elif [ "$LICENCE" = CanUniv -o \( -z "$LICENCE" -a -z "$NO_SOURCE" \) ]; then
   echo Keeping only Canadian University licence info.
   run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| \
           grep -v -x PORTAGEshared/LICENCE \| xargs rm -f
elif [ "$LICENCE" = BinOnly -o -n "$NO_SOURCE" ]; then
   echo No source: removing all LICENCE files.
   run_cmd find PORTAGEshared -name LICENCE\* -maxdepth 1 \| xargs rm -f
else
   error_exit "Invalid -licence specfication"
fi

run_cmd rm -f PORTAGEshared/make-distro.sh

}
#skip
run_cmd cd $OUTPUT_DIR

run_cmd rsync -arz ilt.iit.nrc.ca:/opt/Lizzy/PORTAGEshared/snapshot/ \
                   PORTAGEshared/doc/user-manual

run_cmd find PORTAGEshared/doc/user-manual/uploads -name Layout* \| xargs rm -f
run_cmd 
