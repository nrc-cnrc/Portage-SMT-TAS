#!/bin/bash

# run_all.sh - run the whole test suite
#
# PROGRAMMER: Eric Joanis / Samuel Larkin
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
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
echo Saving log to times_all.txt.  Put this script in the background and use
echo '"tail -f times_all.txt"' if you want to monitor it.

# The rest of this script's output goes to times_all.txt, so we place it in
# parenthese so we don't have to rewrite the redirection on each line.
for lang in $LANGUAGES; do
(
   echo $LANG_MSG

   for script in [0-9][0-9]_*.pl;
   do
      echo
      date
      echo ./$script $lang
      time ./$script $lang
      echo
      echo
   done

   echo
   date
) >& times_all.$lang.txt
done
