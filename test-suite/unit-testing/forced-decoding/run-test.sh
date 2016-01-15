#!/bin/bash

make clean
if [[ $PBS_JOBID ]]; then
   # When running in a PBS job, we need at least 2 workers.  run-parallel.sh
   # will automatically submit a subjob if it's not already running in a two-CU
   # job, but won't otherwise.
   run-parallel.sh -psub -2 -e 'make all -B' 1
elif [[ $GECOSHEP_JOB_ID ]]; then
   # What we really need is just more memory, so we ask for that instead of a cluster that
   # supports it.
   run-parallel.sh -psub "-mem 9" -e 'make all -B' 1
else
   make all -B
fi
