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

make clean
make gitignore

set -o errexit
set -v

readonly TOY_SYSTEM=$PORTAGE/test-suite/systems/toy-regress-en2fr
readonly INCREMENTAL_UPDATE=$HOME/sandboxes/PORTAGEshared/src/utils/incremental-update.sh

ln -sf $TOY_SYSTEM/models .
#cp $TOY_SYSTEM/canoe.ini.cow canoe.ini.cow

utokenize.pl -noss -lang en < s1 | utf8_casemap -c l | canoe-escapes.pl > s1.rules

# Create an empty incremental model, with only a dummy corpus
echo '__DUMMY__ __DUMMY__	__DUMMY__ __DUMMY__' > corpora
time $INCREMENTAL_UPDATE -v en fr corpora >& log.update-dummy
time canoe -f canoe.ini.incr -input s1.rules > s1.out 2> log.s1.out

# Create a model with the s1 t1 sentence pair update, and redecode to see
# "Mr. Boazek" correctly translated to "M. Baozeck".
paste s1 t1 > corpora
time $INCREMENTAL_UPDATE -v en fr corpora >& log.update-s1t1
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
   flock --shared $UPDATE_LOCK_FD
   time canoe -f canoe.ini.incr -input dev1_en.rules > dev1.out.0 2> log.dev1.out.0
   sleep 5
   flock --unlock $UPDATE_LOCK_FD
) &

# to speed up the unit test, updates are only actually done $UPDATE_FREQ lines at a time.
UPDATE_FREQ=12
cat < /dev/null > dev1.out.incr
for l in $(seq 1 `wc -l < dev1_fr.raw`); do
   time select-line $l dev1_en.rules | canoe -f canoe.ini.incr \
      >> dev1.out.incr 2> log.dev1.out.incr-$l
   if (($l / $UPDATE_FREQ * $UPDATE_FREQ == $l)); then
      echo Doing incremental update using $l lines
      paste <(head -$l dev1_en.raw) <(head -$l dev1_fr.raw) > corpora
      time $INCREMENTAL_UPDATE -v en fr corpora >& log.update-$l
   fi
   #time canoe -f canoe.ini.incr -input dev1_en.rules > dev1.out.$l 2> log.dev1.out.$l
done

bleumain dev1.out.0 dev1_fr.tok > log.dev1.out.0.bleu
bleumain dev1.out.incr dev1_fr.tok > log.dev1.out.incr.bleu
