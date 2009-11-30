#!/bin/bash
# run-test.sh - Run this test suite, with a non-zero exit status if it fails.
# Tests
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

strip_non_printing < test.strip_non_printing | diff - test.strip_non_printing.ref

exit
