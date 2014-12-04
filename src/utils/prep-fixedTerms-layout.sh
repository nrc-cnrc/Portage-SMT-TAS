#!/bin/bash

# @file prog.sh
# @brief Briefly describe your script here.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

# Change the program name and year here
print_nrc_copyright prep-fixedTerms-layout.sh 2014

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG <translation system's root directory>

  Creates a placeholder for translation systems will make use of fixed terms.

Options:

  -h(elp)     print this help message
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

while [ $# -gt 0 ]; do
   case "$1" in
   -d|-debug)           DEBUG=1;;
   -h|-help)            usage;;
   --)                  shift; break;;
   -*)                  error_exit "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

test $# -eq 0   && error_exit "Missing translation system's root directory."
DESTINATION=$1; shift
test $# -gt 0   && error_exit "Superfluous arguments $*"

if [ $DEBUG ]; then
   echo "
   DEBUG=$DEBUG
   DESTINATION=$DESTINATION
   REMAINING ARGS=$*
" >&2
fi

# Systematicaly create the plugins directory.
mkdir -p $DESTINATION/plugins
[[ `find $DESTINATION/plugins -type f` ]] && chmod 755 $DESTINATION/plugins/*

# Create a world writable fixedTerms directory to allow web user to update fixed terms.
mkdir -p $DESTINATION/plugins/fixedTerms
chmod 777 $DESTINATION/plugins/fixedTerms

# Make sure there is a language model that is world readable.
if [[ ! -s $DESTINATION/plugins/fixedTerms/lm ]]; then
   echo -e '\n\\data\\\nngram 1=2\n\n\\1-grams:\n0\t</s>\n-99\t<s>\n\n\\end\\' > $DESTINATION/plugins/fixedTerms/lm
fi
chmod 644 $DESTINATION/plugins/fixedTerms/lm

# Make sure that if there is a translation model that it is world writable.
[[ -e $DESTINATION/plugins/fixedTerms/tm ]] && chmod 777 $DESTINATION/plugins/fixedTerms/tm


exit 0
