#!/bin/bash
# run-test.sh - Unit-test for parallelize.pl.
# Tests
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

# Control where the 36000 temporary files get created...
if [[ -d /space/group/nrc_ict/project/ ]]; then
   # On the GPSC, we want the faster NFS file system, if we can get to it.
   export TMP_DIR=/space/group/nrc_ict/project/`whoami`
   mkdir -p $TMP_DIR
else
   export TMP_DIR=.
fi

make clean
make all -B -j 3

exit
