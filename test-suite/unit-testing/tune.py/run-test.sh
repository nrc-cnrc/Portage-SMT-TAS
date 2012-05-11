#!/bin/bash
# $Id$

# @file run-test.sh
# @brief Run this test suite, with a non-zero exit status if it fails
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

make clean

if [[ $PBS_JOBID ]] && (( `ulimit -v` < 20000000 )); then
   run-parallel.sh -v -nolocal -psub "-memmap 12" -e 'make testsuite' 1
else
   make testsuite
fi

exit
