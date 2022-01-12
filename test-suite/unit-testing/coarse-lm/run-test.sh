#!/bin/bash

# @file run-test.sh
# @brief Run this test suite, with a non-zero exit status if it fails
#
# @author Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

if ! which-test.sh ngram-count; then
    echo SKIPPED: ngram-count is not installed, skipping this test suite.
    exit 3
fi

make clean
make all

exit
