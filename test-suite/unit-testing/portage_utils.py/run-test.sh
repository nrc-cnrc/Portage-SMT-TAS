#!/bin/bash
# run-test.sh - Unit-test for portage_utils.pyc
# Tests
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, 2022, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, 2022, Her Majesty in Right of Canada

# Run the test suite with Python 2.7
make clean
make all -B -j ${OMP_NUM_THREADS:-$(nproc)} --makefile Makefile.python2

# Run the test suite with Python 3
make clean
make all -B -j ${OMP_NUM_THREADS:-$(nproc)}

exit
