#!/bin/bash

# @file run-mkcls.sh
# @brief Run the mkcls program on multiple, possible compressed, input files.
#
# @author Eric Joanis; updated by Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, 2015, Her Majesty in Right of Canada

# Usage: run-mkcls.sh <mkcls options except -p> [opt] input files
# mkcls requires that its input be a single non-compressed actual file.
# Here we use process substitution to give it multiple, possibly compressed
# files.

# Includes NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh || { echo "Error: Unable to source sh_utils.sh" >&2; exit 1; }

[[ $PORTAGE_INTERNAL_CALL ]] ||
print_nrc_copyright run-mkcls.sh 2008

usage() {
   for msg in "$@"; do
      echo $msg >&2
   done
   [[ $0 =~ [^/]*$ ]] && PROG=$BASH_REMATCH || PROG=$0
   cat <<==EOF== >&2

Usage: $PROG <mkcls options except -p> [opt] <input files>

  Call mkcls with one or more possibly compressed input files.

Options:

  -h(elp)     print this help message

==EOF==

   exit 1
}

mkclsopts=""
while [ $# -gt 0 ]; do
   case $1 in
   -h|-help) usage;;
   --)       shift; break;;
   -*)       mkclsopts="$mkclsopts $1"; shift;;
   opt)      shift; break;;
   *)        break;;
   esac
done

input_files=$*

mkcls $mkclsopts -p<(gzip -cqdf $input_files) opt
