#!/bin/bash
# @file run-test.sh
# @brief Run this test suite, with a non-zero exit status if it fails
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

make clean
make all -j 2 -B

exit

