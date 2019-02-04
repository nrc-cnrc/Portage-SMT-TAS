#!/bin/bash

# @file test-theano-gpu-init.sh 
# @brief Tests whether theano can successfully initialize the cuda device.
# 
# @author Darlene Stewart
# 
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2019, Sa Majeste la Reine du Chef du Canada /
# Copyright 2019, Her Majesty in Right of Canada

# Tests whether theano can successfully initialize the cuda device.
# This script uses the THEANO_FLAGS value from the environment if THEANO_FLAGS
# is set there, or THEANO_FLAGS="mode=FAST_RUN,device=cuda,floatX=float32" if
# THEANO_FLAGS is not set in the environment.
#
# This script exits with a non-zero value if the cuda device was not
# successfully initialized.

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

Usage: $PROG

  Tests whether theano can successfully initialize the cuda device.

  This script uses the THEANO_FLAGS value from the environment if THEANO_FLAGS
  is set there, or THEANO_FLAGS="mode=FAST_RUN,device=cuda,floatX=float32" if
  THEANO_FLAGS is not set in the environment.

  This script exits with a non-zero value if the cuda device was not
  successfully initialized.

Options:

  -h(elp)     print this help message
  -v(erbose)  increment the verbosity level by 1 (may be repeated)
  -d(ebug)    print debugging information

==EOF==

   exit 1
}

# Command line processing
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

[[ $# -gt 0 ]]  && error_exit "Superfluous arguments $*"

theano_flags="$THEANO_FLAGS"
if [[ -z $theano_flags ]]; then
   theano_flags="mode=FAST_RUN,device=cuda,floatX=float32"
fi

if [[ $VERBOSE -gt 0 ]]; then
   echo "Running with THEANO_FLAGS=\"$theano_flags\" " >&2
fi

# Produce the output.
THEANO_FLAGS=$theano_flags python -c "import theano"

# Exit with non-zero status if the GPU was not initialized correctly.
THEANO_FLAGS="$theano_flags" python -c "import theano" \
   |& grep -q "Mapped name None to device cuda"
if [[ $? -gt 0 ]]; then
   error_exit "Failed to initialize GPU (device=cuda) correctly."
fi

exit 0
