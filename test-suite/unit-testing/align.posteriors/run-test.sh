#!/bin/bash
# run-test.sh - Run this test suite, with a non-zero exit status if it fails
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

DIFF_IGNORE="--ignore-blank-lines --ignore-matching-lines=TTable --ignore-matching-lines=NRC-CNRC --ignore-matching-lines=portage_info"
./align-cmd.sh 2>&1 |
   sed 's/in [0-9][0-9]* second/in N second/' |
   diff ${DIFF_IGNORE} ref/align.out -
exit
