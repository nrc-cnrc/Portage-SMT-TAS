#!/bin/bash

# @file run-test.sh
# @brief Run this test suite, with a non-zero exit status if it fails
#
# @author Eric Joanis
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

make clean
make gitignore

set -o errexit
set -v

TOY_SYSTEM=$PORTAGE/test-suite/systems/toy-regress-en2fr

# To execute the src instead of bin version of incremental-update.sh, use, e.g.:
#   PATH=$HOME/sandboxes/PORTAGEshared/src/utils:$PATH ./run-test.sh

ln -sf $TOY_SYSTEM/models .
#cp $TOY_SYSTEM/canoe.ini.cow canoe.ini.cow

utokenize.pl -noss -lang en < s1 | utf8_casemap -c l | canoe-escapes.pl > s1.rules

# Create an empty incremental model, with only a dummy corpus
echo `date +"%F %T"`'	__DUMMY__ __DUMMY__	__DUMMY__ __DUMMY__' > corpora
timeFormat "incr-update DUMMY"
time incremental-update.sh -f config -v en fr corpora >& log.update-dummy
timeFormat "canoe DUMMY"
time canoe -f canoe.ini.incr -input s1.rules > s1.out 2> log.s1.out

# Create a model with the s1 t1 sentence pair update, and redecode to see
# "Mr. Boazek" correctly translated to "M. Baozeck".
paste <(echo `date +"%F %T"`) s1 t1 > corpora
timeFormat "incr-update s1"
time incremental-update.sh -f config -v en fr corpora >& log.update-s1t1
timeFormat "canoe s1"
time canoe -f canoe.ini.incr -input s1.rules > s1.out.incr 2> log.s1.out.incr
grep -i Baozeck s1.out.incr

# Prepare the dev1 files for passing to the decoder.
head -30 $PORTAGE/test-suite/tutorial-data/dev1_en.raw > dev1_en.raw
head -30 $PORTAGE/test-suite/tutorial-data/dev1_fr.raw > dev1_fr.raw
utokenize.pl -noss -lang en < dev1_en.raw | utf8_casemap -c l > dev1_en.tok
utokenize.pl -noss -lang fr < dev1_fr.raw | utf8_casemap -c l > dev1_fr.tok
canoe-escapes.pl < dev1_en.tok > dev1_en.rules

# Translate the dev set with the base model, with an empty incremental set.
# Do it in the background with a read lock, to exercise incremental-update.sh
# having to wait on its write lock to do the update.
# TODO: write a test for that... we see it works because the first update takes
# 8 seconds instead of 5, which is due to waiting for the sleep 5 I inserted
# here.  But how do I automate checking that?
(
   readonly UPDATE_LOCK_FD=202
   readonly UPDATE_LOCK_FILE=incremental-update.lock
   eval "exec $UPDATE_LOCK_FD>$UPDATE_LOCK_FILE"
   for n in 1 2; do
      until flock --nonblock --shared $UPDATE_LOCK_FD; do
         echo "canoe.0-$n failed to acquire incremental-update.lock... retrying in 1 second." >&2
         sleep 1
      done
      echo "canoe.0-$n acquired incremental-update.lock" >&2
      timeFormat canoe.0-$n
      time canoe -f canoe.ini.incr -input dev1_en.rules > dev1.out.0-$n 2> log.dev1.out.0-$n
      if [[ $n == 1 ]]; then
         ln -sf dev1.out.0-1 dev1.out.0
         echo "canoe.0-$n keeping incremental-update.lock for 5 more seconds." >&2
         sleep 5
      fi
      echo "canoe.0-$n releasing incremental-update.lock" >&2
      flock --unlock $UPDATE_LOCK_FD
   done
) &

# to speed up the unit test, updates are only actually done $UPDATE_FREQ lines at a time.
UPDATE_FREQ=12
cat < /dev/null > dev1.out.incr
for l in $(seq 1 `wc -l < dev1_fr.raw`); do
   timeFormat canoe.$l
   time select-line $l dev1_en.rules \
      | canoe -f canoe.ini.incr \
      >> dev1.out.incr 2> log.dev1.out.incr-$l
   if (($l % $UPDATE_FREQ == 0)); then
      echo Doing incremental update using $l lines
      paste <(yes `date +"%F %T"` | head -$l) <(head -$l dev1_en.raw) <(head -$l dev1_fr.raw) > corpora
      timeFormat "incr-update $l"
      time incremental-update.sh -f config -v en fr corpora >& log.update-$l
   fi
   #time canoe -f canoe.ini.incr -input dev1_en.rules > dev1.out.$l 2> log.dev1.out.$l
done

bleumain dev1.out.0 dev1_fr.tok > log.dev1.out.0.bleu
bleumain dev1.out.incr dev1_fr.tok > log.dev1.out.incr.bleu
grep Human *.bleu
