#!/bin/bash

# run_all.sh - run the whole test suite
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Conseil national de recherches du Canada / Copyright 2006, National Research Council of Canada

echo 'regress-small-voc, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada'

usage() {
   for msg in "" "$@"; do
      echo $msg >&2
   done
   cat <<==EOF== >&2
Usage: run_all.sh [-h(elp)] [languages]

   Run the whole test suite on the specified languages, which may be one or
   more of fr, de, es, and fi.  [fr]

==EOF==
   exit 1
}

while [ $# -gt 0 ]; do
   case "$1" in
   -h|-help)            usage;;
   -*)                  usage "Unknown option $1.";;
   *)                   break;;
   esac
   shift
done

LANGUAGES=$*

if [ -n "$LANGUAGES" ]; then
   LANG_MSG="Processing languages: $LANGUAGES"
else
   LANG_MSG="Processing default languages"
fi

# Echo this here and in the parens, so it's on stdout as well as in the log.
echo $LANG_MSG
echo Saving log to times.txt.  Put this script in the background and use
echo '"tail -f times.txt"' if you want to monitor it.

# The rest of this script's output goes to times.txt, so we place it in
# parenthese so we don't have to rewrite the redirection on each line.
(
   echo $LANG_MSG

   # removed/obsolete:
   #   07_rcanoe    \   # decode and gen ff on dev - replaced by 28_rat_train
   #   08_rcanoe    \   # decode and gen ff on test - replaced by 29_rat_test
   #   10_cow       \   # COW with multiple phrase tables
   #   21_rtrain    \   # rescore_train on dev - replaced by 28_rat_train
   #   22_sanity    \   # "test" on dev, to make sure things work ok
   #   23_translate \   # rescore_translate on test - replaced by 29_rat_test

   # main suite:
   #   02_train_ibm \   # build IBM translation tables
   #   03_gen_phr   \   # build phrase tables
   #   04_canoe     \   # simple decoding run
   #   11_cow_mp    \   # COW, optimize canoe weights, with a multi-prob table
   #   28_rat_train \   # train the rescoring model
   #   29_rat_test  \   # test using the rescording model
   #   30_summary   \   # summarize bleu scores on dev/test, 1-best/rescore.

   for script in   \
      02_train_ibm \
      03_gen_phr   \
      04_canoe     \
      10_cow       \
      28_rat_train \
      29_rat_test  \
      30_summary   \
   ; do
      echo
      date
      echo ./$script.pl $LANGUAGES
      time ./$script.pl $LANGUAGES
   done

   echo
   date
) >& times.txt
