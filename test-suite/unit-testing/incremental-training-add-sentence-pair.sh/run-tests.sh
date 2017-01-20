#!/bin/bash
# @file run-tests.sh
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

readonly BoldRed='\e[01;31m'
readonly Red='\e[0;31m'
readonly Bold='\e[1m'
readonly Reset='\e[00m'

trap "trap - EXIT; echo Test FAILED" ERR
trap "echo All tests PASSED." EXIT


readonly INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR=${INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR:-incremental-training-add-sentence-pair.sh}

readonly CORPORA=corpora
readonly QUEUE=queue
readonly LOG=log
readonly MAX_CONCURRENT_CALLS=25
readonly INCREMENTAL_TRAINING_SCRIPT=./incremental_training.sh
readonly DELAY=1
readonly TRAINING_TIME=12
readonly WITNESS=witness
export TRAINING_TIME
export WITNESS


# Helper function to make sure we start a testcase from a clean environment.
function cleanup() {
   rm -f $CORPORA
   rm -f $QUEUE
   rm -f $LOG*
   rm -f $WITNESS
}



function testcaseDescritption() {
   echo -e "${Bold}Testcase:${Reset} $@" >&2
}



function error_message() {
   echo -e "${BoldRed}Error: ${Red}$@${Reset}" >&2
}



function add_sentence_pair_to_queue() {
   echo "Manually adding a sentence pair to the queue" >&2
   echo -e "source\ttarget" > $QUEUE

   echo "Making sure the queue holds a sentence pair" >&2
   [[ `wc -l < $QUEUE` -eq 1 ]] \
   || ! error_message "The queue is corrupted"
}



function trigger_single_training() {
   $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR -verbose -unittest  "$INCREMENTAL_TRAINING_SCRIPT" >&2 &
}



function no_script() {
   set -o errexit
   testcaseDescritption "No incremental training script"
   $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR -verbose -unittest \
   |& grep --quiet 'Error: You must provide an incremental training script.' \
   || ! error_message "There should be an error message about missing incremental training script missing."
}



function unable_to_write_to_the_queue(){
   set -o errexit
   testcaseDescritption "Unable to write to the queue"
   cleanup
   touch $QUEUE
   chmod a-w $QUEUE
   $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR \
      -verbose \
      -unittest \
      "$INCREMENTAL_TRAINING_SCRIPT" \
      "$i-S-$$ `date +'%T:%N'`" \
      "$i-T-$$ `date +'%T:%N'`" \
   |& grep --quiet 'Error writing sentence pair to the queue' \
   || ! error_message "We shouldn't be able to write to the queue."
}



function multiple_add_and_multiple_trigger() {
   testcaseDescritption "Multiple add and multiple training triggers."
   cleanup

   # Let's prepopulate the queue with at least one sentence pair and then let
   # whom ever between add or trigger to start the training process.
   add_sentence_pair_to_queue

   # Let's start $MAX_CONCURRENT_CALLS to add some random sentence pairs at different time.
   echo "Adding $MAX_CONCURRENT_CALLS sentence pairs" >&2
   for i in `seq --equal-width $MAX_CONCURRENT_CALLS`;
   do
      # Add sentence pair and trigger training.
      {
         sleep $DELAY;
         $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR \
            -verbose \
            -unittest \
            "$INCREMENTAL_TRAINING_SCRIPT" \
            "$i-S-$$ `date +'%T:%N'`" \
            "$i-T-$$ `date +'%T:%N'`" \
            &> $LOG.add.$i;
      } &
      # Trigger training only.
      {
         sleep $DELAY;
         $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR \
            -verbose \
            -unittest \
            "$INCREMENTAL_TRAINING_SCRIPT" \
            &> $LOG.trigger.$i;
      } &
   done

   echo "Waiting for everyone to be done" >&2
   # Because $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR disowns the training
   # script, it is not sufficient to simply `wait` we need to sleep.
   sleep $(( $DELAY + 2 * $TRAINING_TIME + 1 ))

   echo "Validating..." >&2
   set -o errexit

   [[ `find -name $LOG.\* | wc -l` -eq $((2 * $MAX_CONCURRENT_CALLS)) ]] \
   || ! error_message "We should have got $((2 * $MAX_CONCURRENT_CALLS)) log files."

   [[ `wc -l < $CORPORA` -eq $(($MAX_CONCURRENT_CALLS + 1)) ]] \
   || ! error_message "We should have got $(($MAX_CONCURRENT_CALLS + 1)) sentence pairs in the corpus."

   # Let's make sure the sentence pairs have the format that we expect.
   [[ `grep --count --perl-regexp '^\d{2}-S-\d{1,5} \d{2}:\d{2}:\d{2}:\d{1,9}\t\d{2}-T-\d{1,5} \d{2}:\d{2}:\d{2}:\d{1,9}' $CORPORA` -eq $MAX_CONCURRENT_CALLS ]] \
   || ! error_message "Some sentence pairs are corrupted."

   [[ `wc -l < $QUEUE` -eq 0 ]] \
   || ! error_message "All elements of the queue should have been procesed."

   # All except one process should claim that training is already ongoing.
   [[ `grep 'Training is already in progress' $LOG.* | wc -l` -eq $((2 * $MAX_CONCURRENT_CALLS - 1)) ]] \
   || ! error_message "All except one process should claim that training is already ongoing."

   # We expect the first process to add to queue will start training then, the
   # same process will have to redo training since the other 9 processes will have
   # added their sentence pair.
   [[ `grep --files-with-matches 'is training' $LOG.* | wc -l` -eq 1 ]] \
   || ! error_message "Only one training should be ongoing at once."

   [[ `grep --count 'is training' $LOG.* | grep --count --invert-match ':0$'` -eq 1 ]] \
   || ! error_message "Only one process should have trained multiple times."

   [[ `wc -l < $WITNESS` -eq 2 ]] \
   || ! error_message "The witness should report 2."
}



function insert_and_multiple_trigger_training() {
   testcaseDescritption "Multiple simultaneous triggers for training."
   cleanup
   add_sentence_pair_to_queue

   # Let's trigger training.
   echo "Triggering $MAX_CONCURRENT_CALLS incremental training" >&2
   for i in `seq --equal-width $MAX_CONCURRENT_CALLS`;
   do
      {
         sleep $DELAY;
         $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR \
            -verbose \
            -unittest \
            "$INCREMENTAL_TRAINING_SCRIPT" \
            &> $LOG.trigger.$i;
      } &
   done

   echo "Waiting for everyone to be done" >&2
   # Because $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR disowns the training
   # script, it is not sufficient to simply `wait` we need to sleep.
   sleep $(( $DELAY + $TRAINING_TIME + 1 ))

   echo "Validating..." >&2
   set -o errexit

   [[ `grep --files-with-matches 'is training' $LOG.trigger.* | wc -l` -eq 1 ]] \
   || ! error_message "Only one training should be ongoing at once."

   # All except one process should claim that training is already ongoing.
   [[ `grep 'Training is already in progress' $LOG.trigger.* | wc -l` -eq $(($MAX_CONCURRENT_CALLS - 1)) ]] \
   || ! error_message "Only one process should have trained multiple times."

   [[ `wc -l < $WITNESS` -eq 1 ]] \
   || ! error_message "The witness should report 1."
}



which $INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR &> /dev/null \
|| { ! error_message "INCREMENTAL_TRAINING_ADD_SENTENCE_PAIR not defined"; exit 1; }

no_script
unable_to_write_to_the_queue
multiple_add_and_multiple_trigger
insert_and_multiple_trigger_training

