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

# To execute the src instead of bin version of incr-update.sh, use, e.g.:
#   PATH=$HOME/sandboxes/PORTAGEshared/src/utils:$PATH ./run-test.sh

ln -sf $TOY_SYSTEM/models .
#cp $TOY_SYSTEM/canoe.ini.cow canoe.ini
#create canoe.ini.incr from canoe.ini

utokenize.pl -noss -lang en < s1 | utf8_casemap -c l | canoe-escapes.pl > s1.rules

# Create an empty incremental model, with only a dummy corpus
echo `date +"%F %T"`'	__DUMMY__ __DUMMY__	__DUMMY__ __DUMMY__' > corpora
timeFormat "incr-update DUMMY"
time incr-update.sh -f config -v corpora >& log.update-dummy
timeFormat "canoe DUMMY"
time canoe -f canoe.ini.incr -input s1.rules > s1.out 2> log.s1.out

# Create an incremental model with too small a target corpus for regular LM
# creation
echo `date +"%F %T"`'	.	.' > corpora
timeFormat "incr-update toosmall"
time incr-update.sh -f config -v corpora >& log.update-toosmall
grep -q __DUMMY__ lm.incremental_fr

# Create a model with the s1 t1 sentence pair update, and redecode to see
# "Mr. Boazek" correctly translated to "M. Baozeck".
paste <(echo `date +"%F %T"`) s1 t1 > corpora
timeFormat "incr-update s1"
time incr-update.sh -f config -v corpora >& log.update-s1t1
timeFormat "canoe s1"
time canoe -f canoe.ini.incr -input s1.rules > s1.out.incr 2> log.s1.out.incr
grep -i Baozeck s1.out.incr

# Use custom commands in the config and make sure they work
timeFormat "incr-update custom cmds"
time incr-update.sh -f config.custom-cmds -v corpora >& log.update-custom
grep -q Boazek cpt.incremental.en2fr
grep -q KCEZOAB cpt.incremental.en2fr
grep -q "ERTSINIM UA ELLEVUON" lm.incremental_fr

# Prepare the dev1 files for passing to the decoder.
head -30 $PORTAGE/test-suite/tutorial-data/dev1_en.raw > dev1_en.raw
head -30 $PORTAGE/test-suite/tutorial-data/dev1_fr.raw > dev1_fr.raw
utokenize.pl -noss -lang en < dev1_en.raw | utf8_casemap -c l > dev1_en.tok
utokenize.pl -noss -lang fr < dev1_fr.raw | utf8_casemap -c l > dev1_fr.tok
canoe-escapes.pl < dev1_en.tok > dev1_en.rules

# Translate the dev set with the base model, with an empty incremental set.
# Do it in the background with a shared lock (acquired by canoe itself), to
# exercise incr-update.sh having to wait on its exclusive lock to do the update.
# TODO: write a test for that... we see it works because the updates are
# delayed to happen between individual canoe.0-i iterations.  But how do I
# automate checking that?
(
   for n in 1 2 3 4 5; do
      timeFormat canoe.0-$n
      echo "canoe.0-$n about to ask for lock on canoe.ini.incr on" `date` >&2
      time canoe -v 2 -f canoe.ini.incr -input dev1_en.rules > dev1.out.0-$n 2> log.dev1.out.0-$n
      echo "canoe.0-$n just released lock on canoe.ini.incr on" `date` >&2
      if [[ $n == 1 ]]; then
         ln -sf dev1.out.0-1 dev1.out.0
      fi
   done
) &

# to speed up the unit test, updates are only actually done $UPDATE_FREQ lines at a time.
UPDATE_FREQ=12
cat < /dev/null > dev1.out.incr
for l in $(seq 1 `wc -l < dev1_fr.raw`); do
   timeFormat canoe.$l
   time select-line $l dev1_en.rules \
      | canoe -v 2 -f canoe.ini.incr \
      >> dev1.out.incr 2> log.dev1.out.incr-$l
   if (($l % $UPDATE_FREQ == 0)); then
      echo Doing incremental update using $l lines
      paste <(yes `date +"%F %T"` | head -$l) <(head -$l dev1_en.raw) <(head -$l dev1_fr.raw) > corpora
      timeFormat "incr-update $l"
      time incr-update.sh -f config -v corpora >& log.update-$l
   fi
   #time canoe -f canoe.ini.incr -input dev1_en.rules > dev1.out.$l 2> log.dev1.out.$l
done

bleumain dev1.out.0 dev1_fr.tok > log.dev1.out.0.bleu
bleumain dev1.out.incr dev1_fr.tok > log.dev1.out.incr.bleu
grep Human *.bleu

wait
