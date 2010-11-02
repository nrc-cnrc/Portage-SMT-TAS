#!/bin/bash
# run-test.sh - Run this test suite, which validates the process by which run-parallel.sh
#               asseses how many local workers are launched. 
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

./go.sh 2>&1 |
   egrep "(NOLOCAL|PBS_JOBID|JOB_VMEM|PARENT_VMEM|NCPUS|PARENT_NCPUS|LOCAL_JOBS|FIRST_PSUB|NUM)" |
   diff -b ref -

