#!/bin/bash
# run-test.sh - Run this test suite, with a non-zero exit status if it fails
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

rc=

make clean
make -j 2 || rc=1
make -j 2 EXTRA_CANOE_ARGS="-ttable-prune-type full" || rc=1

if [[ $rc ]]; then
   echo At least one of the two top-level variants failed.
fi
exit $rc
