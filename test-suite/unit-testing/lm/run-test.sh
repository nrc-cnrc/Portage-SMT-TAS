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

# Prevent arpalm2tplm.sh from using psub, since we're only doing small LMs that
# are slower parallelized on the cluster than done sequentially.
export PORTAGE_NOCLUSTER=1

make clean
make -B -j 2
