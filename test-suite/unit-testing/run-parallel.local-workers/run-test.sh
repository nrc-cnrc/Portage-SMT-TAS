#!/bin/bash
# run-test.sh - Run this test suite, which validates the process by which
#               run-parallel.sh asseses how many local workers are launched.
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


make clean
make gitignore
./go-2.sh
COUNTER=1
while true; do
   ALL_EXIST=1
   for FILENO in `seq -w 01 14`; do
      if [[ ! -s out.$FILENO ]]; then
         echo "Waiting for output file(s), starting with out.$FILENO"
         ALL_EXIST=
         break
      fi
   done
   if [[ $ALL_EXIST ]]; then
      echo "Got all output files, waiting a bit more to make sure they're fully written"
      # 5 seconds is sometimes not enough on the GPSC; crank up to 10.
      sleep 10
      break
   fi
   if [[ $COUNTER -gt 100 ]]; then
      echo "Still waiting on output files after $((COUNTER*60)) seconds. Giving up."
      break
   fi
   COUNTER=$((COUNTER+1))
   sleep 5
done

cat out.[01][0-9] |
   egrep "(NOLOCAL|JOB_VMEM|PARENT_VMEM|NCPUS|PARENT_NCPUS|LOCAL_JOBS|FIRST_PSUB|NUM)" |
   perl -ple 's#(VMEM *= *)(\d{4,})#$1.$2/1024#e' |
   diff -w ref-2 -
if [[ $? = 0 ]]; then
   echo All tests PASSED
   exit 0
else
   echo Test FAILED
   exit 1
fi

#./go.sh 2>&1 |
#   egrep "(NOLOCAL|PBS_JOBID|JOB_VMEM|PARENT_VMEM|NCPUS|PARENT_NCPUS|LOCAL_JOBS|FIRST_PSUB|NUM)" |
#   diff -b ref -

