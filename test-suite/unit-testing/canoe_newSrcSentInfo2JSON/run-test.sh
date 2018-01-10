#!/bin/bash
# @file run-test.sh
# @brief Run this unittest.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada


make clean

if which-test.sh jq; then
   make
   exit
else
   echo 'ERROR: jq required to run this test suite: https://stedolan.github.io/jq/'
   exit 1
fi
