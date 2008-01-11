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
(
   echo $LANG_MSG

   # More extensive test suite - run every test script in the suite
   # See run_minimum.sh for a description of the basic suite
   # Additional scripts are:
   #   20_canoe.pl           - simple decoding run
   #   21_sanity.pl          - "test" on dev, to make sure things work ok
   #   22_gen_phr.pl         - variants on phrase table building
   #   25_cow_ext.pl         - variants on cow
   #   30_rtrain_ebsc.pl     - train the rescoring model with the
   #                           expectation-based stopping criterion
   #   31_rtrain_rnd_w.pl    - test the random weight distribution specs
   #   40_phrase_tm_align.pl - test phrase_tm_align
   #   50_filter_models.pl   - test the various modes of filter_models
   #   61_canoe_cube.pl      - test the cube pruning decoder
   #   62_cow_cube.pl        - test COW with cube pruning

   for script in [0-9][0-9]_*.pl;
   do
      echo
      date
      echo ./$script $LANGUAGES
      time ./$script $LANGUAGES
      echo
      echo
   done

   echo
   date
) >& times_all.txt
