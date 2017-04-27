#!/bin/bash
# run-test.sh - Unit-test for translate.pl
# Tests
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

# This test suite is much faster in non-cluster mode because cluster overhead dominates
# the runtime otherwise
export PORTAGE_NOCLUSTER=1

make clean
make all -B -j 2 -k
