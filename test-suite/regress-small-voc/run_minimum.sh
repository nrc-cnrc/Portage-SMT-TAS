#!/bin/bash

# run_all.sh - run the minimum required to get rescored translations
#
# PROGRAMMER: Samuel Larkin
#
# COMMENTS:
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005 - 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005 - 2008, Her Majesty in Right of Canada

echo 'regress-small-voc, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada'

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
echo Saving log to times_minimum.txt.  Put this script in the background and use
echo '"tail -f times_minimum.txt"' if you want to monitor it.

# The rest of this script's output goes to times_minimum.txt, so we place it in
# parenthese so we don't have to rewrite the redirection on each line.
(
   echo $LANG_MSG

   # Minimal test suite - run the basics from start to finish
   #   02_train_ibm.pl  - build IBM translation tables
   #   03_gen_phr.pl    - build phrase tables
   #   05_cow.pl        - COW, optimize canoe weights
   #   10_rat_train.pl  - train the rescoring model
   #   11_rat_test.pl   - test using the rescording model
   #   18_summary.pl    - summarize bleu scores on dev/test, 1-best/rescore.

   for script in [01][0-9]_*.pl;
   do
      echo
      date
      echo ./$script $LANGUAGES
      time ./$script $LANGUAGES
   done

   echo
   date
) >& times_minimum.txt
