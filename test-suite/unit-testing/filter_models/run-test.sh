#!/bin/bash
# run-test.sh - Run this test suite, with a non-zero exit status if it fails
#
# PROGRAMMER: Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

make clean
make SETUP
make all -j 2
RET=$?

make -C histogram clean
make -C histogram all
RET2=$?
if [[ ! $RET2 == 0 ]]; then
   RET=$RET2
fi

if [[ $RET == 0 ]]; then
   echo "All tests and sub-tests PASSED."
else
   echo ""
   echo "At least one test or sub-test FAILED."
fi

exit $RET
