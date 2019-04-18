#!/bin/bash

make gitignore

if [[ $PBS_JOBID || $GECOSHEP_JOB_ID ]]; then
   run-parallel.sh -v -psub "-mem 16" -c ./madamira-test.sh
   run-parallel.sh -v -psub "-mem 16" -c ./stanseg-test.sh
else
   ./madamira-test.sh
   ./stanseg-test.sh
fi
