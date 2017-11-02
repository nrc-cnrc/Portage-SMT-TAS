#!/bin/bash

# @file run-locking-test.sh
# @brief Run the incr-update locking test, with a non-zero exit status if it fails
#
# Assumptions:
#   - current working directory has been initialized with the incremental model
#   - incremental canoe in file is named canoe.ini.cow
#   - dev1 files are located in ../
#
# @author Eric Joanis and Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada
# Copyright 2017, Her Majesty in Right of Canada

timeFormat()
{
   local tag=$1
   TIMEFORMAT=$'\n'"$tag:  "$'real %3lR\tuser %3lU\tsys %3lS'   
}

set -o errexit
set -v

# Translate the dev set with the base model, with an empty incremental set.
# Do it in the background with a shared lock (acquired by canoe itself), to
# exercise incr-update.sh having to wait on its exclusive lock to do the update.
# TODO: write a test for that... we see it works because the updates are
# delayed to happen between individual canoe.0-i iterations.  But how do I
# automate checking that?
(
   for n in 1 2 3 4 5; do
      timeFormat canoe.0-$n
      echo "canoe.0-$n about to ask for lock on canoe.ini.cow on" `date` >&2
      time canoe -v 2 -f canoe.ini.cow -input ../dev1_en.rule > dev1.out.0-$n 2> log.dev1.out.0-$n
      echo "canoe.0-$n just released lock on canoe.ini.cow on" `date` >&2
      if [[ $n == 1 ]]; then
         ln -sf dev1.out.0-1 dev1.out.0
      fi
   done
) &

# to speed up the unit test, updates are only actually done $UPDATE_FREQ lines at a time.
UPDATE_FREQ=12
cat < /dev/null > dev1.out.incr
for l in $(seq 1 `wc -l < ../dev1_fr.raw`); do
   timeFormat canoe.$l
   time select-line $l ../dev1_en.rule \
      | canoe -v 2 -f canoe.ini.cow \
      >> dev1.out.incr 2> log.dev1.out.incr-$l
   if (($l % $UPDATE_FREQ == 0)); then
      echo Doing incremental update using $l lines
      paste <(yes `date +"%F %T"` | head -$l) <(head -$l ../dev1_en.raw) <(head -$l ../dev1_fr.raw) > corpora
      timeFormat "incr-update $l"
      time incr-update.sh -v corpora >& log.update-$l
   fi
   #time canoe -f canoe.ini.cow -input ../dev1_en.rule > dev1.out.$l 2> log.dev1.out.$l
done

wait
