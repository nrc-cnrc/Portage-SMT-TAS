#!/bin/bash

if [[ $PBS_JOBID || $GECOSHEP_JOB_ID ]]; then
   run-parallel.sh -v -psub "-mem 16" -c ./madamira-test.sh
else
   ./madamira-test.sh
fi
