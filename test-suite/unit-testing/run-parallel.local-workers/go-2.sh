#!/bin/bash
# run-test.sh - Run the various run-parallel.sh commands in this test suite.
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada


psub 'run-parallel.sh -unit-test -d <(seq 1 10) 4 >& out.01'
psub 'run-parallel.sh -unit-test -d <(seq 1 3) 4 >& out.02'
psub -2 'run-parallel.sh -psub -1 -unit-test -d <(seq 1 10) 4 >& out.03'
psub -5 'run-parallel.sh -psub -2 -unit-test -d <(seq 1 10) 4 >& out.04'
psub -5 'run-parallel.sh -psub -2 -unit-test -d <(seq 1 10) 1 >& out.05'
psub -6 'run-parallel.sh -psub "-2 -memmap 4" -unit-test -d <(seq 1 10) 4 >& out.06'
psub -6 'run-parallel.sh -psub "-2 -memmap 4" -unit-test -d <(seq 1 10) 2 >& out.07'
psub -6 'run-parallel.sh -psub "-2 -memmap 4" -unit-test -d <(seq 1 10) 1 >& out.08'
psub -4 -memmap 1 'run-parallel.sh -psub "-1 -memmap 1" -unit-test -d <(seq 1 10) 4 >& out.09'
psub -2 -memmap 20 'run-parallel.sh -psub -3 -unit-test -d <(seq 1 10) 4 >& out.10'
psub -2 -memmap 20 'run-parallel.sh -psub "-memmap 30" -unit-test -d <(seq 1 10) 4 >& out.11'
psub -2 -memmap 4 -j 4 'run-parallel.sh -j 4 -unit-test -d <(seq 1 10) 8 >& out.12'
psub -j 4 -memmap 1 'run-parallel.sh -psub "-1 -memmap 1" -unit-test -d <(seq 1 10) 4 >& out.13'
psub -j 12 -memmap 4 'run-parallel.sh -unit-test -d <(seq 1 20) 12 >& out.14'

