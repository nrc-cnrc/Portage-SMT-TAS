#!/bin/bash

# @file run-mkcls.sh
# @brief run the mkcls program.
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

# Usage: run-mkcls.sh <mkcls options except -p> [opt] input files
# mkcls requires that its input be a single non-compressed actual file.
# Here we use process substitution to give it multiple, possibly compressed
# files.

mkclsopts=""
while [ $# -gt 0 ]; do
   case $1 in
   -h)  echo "Usage: run-mkcls.sh <mkcls options except -p> [opt] <input files>

       Call mkcls with one or more possibly compressed input files.
" >&2; exit;;
   -*)  mkclsopts="$mkclsopts $1"; shift;;
   opt) shift; break;;
   *)   break;;
   esac
done
input_files=$*
mkcls $mkclsopts -p<(gzip -cqdf $input_files) opt
