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

make gitignore

./align-cmd.sh 2>&1 |
   sed 's/in [0-9][0-9]* second/in N second/' |
   diff-round.pl -min -p -1.2 ref/align.out -
exit
