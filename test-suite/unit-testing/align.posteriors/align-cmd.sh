#!/bin/bash

MODELDIR=models
export PORTAGE_INTERNAL_CALL=1

run_cmd() {
   echo "$*"
   eval $*
   RC=$?
   echo RC=$RC
   return $RC
}

#for MODEL in hmm; do
for MODEL in hmm ibm1 ibm2; do
   if [ $MODEL = ibm1 ]; then
      IBM_OPT="-ibm 1"
   else
      IBM_OPT=""
   fi
   # For cutting and pasting into the command below
   # -a 'IBMOchAligner 3 exclude' \
   run_cmd align-words \
      -post \
      -o matrix \
      -v -vv \
      -a "'PosteriorAligner 0.1 0.001'" \
      $IBM_OPT \
      $MODELDIR/$MODEL.en_given_fr \
      $MODELDIR/$MODEL.fr_given_en \
      1line.fr 1line.en

done

