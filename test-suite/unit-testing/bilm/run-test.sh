#!/bin/bash

# @file run-test.sh
# @brief Run this test suite, with a non-zero exit status if it fails
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

#set -o errexit

# Cluster overhead dominates in this suite, so disable it.
export PORTAGE_NOCLUSTER=1

if ! which-test.sh ngram-count; then
    echo SKIPPED: ngram-count is not installed, skipping this test suite.
    exit 3
fi

make clean
make gitignore
BAD=
for pt_type in cpt tppt; do
   export PT_TYPE=$pt_type

   make out err_min_lmc boundary_cases -j 4 || BAD=1
   make en.cls.gz fr.cls.gz lm.tplm bilm.tiny.tplm -j || BAD=1

   #time-mem make -j coarseout cls_en=  cls_fr=  cls_bitoken=  verbosity=2 || BAD=1 # takes about 35s.
   #time-mem make -j coarseout cls_en=1 cls_fr=  cls_bitoken=  verbosity=2 || BAD=1 # takes about 15s.
   #time-mem make -j coarseout cls_en=  cls_fr=1 cls_bitoken=  verbosity=2 || BAD=1 # takes about 15s.
   #time-mem make -j coarseout cls_en=1 cls_fr=1 cls_bitoken=  verbosity=2 || BAD=1 # takes about 15s.
   #time-mem make -j coarseout cls_en=  cls_fr=  cls_bitoken=1 verbosity=2 || BAD=1 # takes about 105s.
   #time-mem make -j coarseout cls_en=1 cls_fr=  cls_bitoken=1 verbosity=2 || BAD=1 # takes about 70s.
   #time-mem make -j coarseout cls_en=  cls_fr=1 cls_bitoken=1 verbosity=2 || BAD=1 # takes about 75s.
   #time-mem make -j coarseout cls_en=1 cls_fr=1 cls_bitoken=1 verbosity=2 || BAD=1 # takes about 55s.

   echo \
   'make coarseout cls_en=  cls_fr=  cls_bitoken=1 verbosity=2 >& log.make001
   make coarseout cls_en=1 cls_fr=  cls_bitoken=1 verbosity=2 >& log.make101
   make coarseout cls_en=  cls_fr=1 cls_bitoken=1 verbosity=2 >& log.make011
   make coarseout cls_en=1 cls_fr=1 cls_bitoken=1 verbosity=2 >& log.make111
   make coarseout cls_en=  cls_fr=  cls_bitoken=  verbosity=2 >& log.make000
   make coarseout cls_en=1 cls_fr=  cls_bitoken=  verbosity=2 >& log.make100
   make coarseout cls_en=  cls_fr=1 cls_bitoken=  verbosity=2 >& log.make010
   make coarseout cls_en=1 cls_fr=1 cls_bitoken=  verbosity=2 >& log.make110' |
      run-parallel.sh -on-error continue - 4 || BAD=1
done

if [[ $BAD ]]; then
   echo At least one test FAILED.
   exit 1
else
   echo All tests PASSED.
   exit 0
fi
