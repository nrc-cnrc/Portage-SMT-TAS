#!/bin/bash
# @file run-test.sh
# @brief SparseModel's testsuite.
#
# @author Eric Joanis & Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada


make gitignore
set -e
set -v
export PORTAGE_INTERNAL_CALL=1

### This part of the test suite is intended to test all features

function testcase1 {
   # initialization test
   [[ -d out1 ]] || mkdir out1
   cp data/sparsemodel out1
   palminer -v -pop -m out1/sparsemodel 2> out1/log.palminer
   diff out1 ref1 -q
   echo "PASS: full sparsemodel initialization"

   # decoding with sparse model and printing all feature values
   [[ -d out2 ]] || mkdir out2
   (canoe -f data/canoe.ini.cow -sfvals -ffvals -stack 1000 \
      < data/test_fr.lc |
      ./split-feature-values.pl > out2/test.out) 2>&1 |
      ./grepout-timing.pl > out2/log.canoe.notiming
   diff out2 ref2 -q
   echo "PASS: canoe with full sparsemodel"
}
testcase1

### This part of the test suite in intended to test only commonly used features

function testcase2 {
   # initialization test
   [[ -d out3 ]] || mkdir out3
   cp data/sparsemodel-small out3/sparsemodel
   palminer -v -pop -m out3/sparsemodel 2> out3/log.palminer
   diff out3 ref3 -q
   echo "PASS: small sparsemodel initialization"

   # decoding with sparse model and printing all feature values
   [[ -d out4 ]] || mkdir out4
   (canoe -f data/canoe.ini.cow -sfvals -ffvals -sparse-model ../ref3/sparsemodel -stack 1000 \
      -sparse-model-allow-non-local-wts \
      < data/test_fr.lc |
      ./split-feature-values.pl > out4/test.out) 2>&1 |
      ./grepout-timing.pl > out4/log.canoe.notiming
   diff out4 ref4 -q
   echo "PASS: canoe with small sparsemodel"
}
testcase2

### And now we go for a fuller set of features, combining both regular and
### LR distortion features.

function testcase3 {
   # initialization test
   [[ -d out5 ]] || mkdir out5
   cp data/sparsemodel-big out5/sparsemodel
   palminer -v -pop -m out5/sparsemodel 2> out5/log.palminer
   diff out5 ref5 -q
   echo "PASS: full sparsemodel initialization"

   # decoding with sparse model and printing all feature values
   [[ -d out6 ]] || mkdir out6
   (canoe -f data/canoe.ini.cow -sfvals -ffvals -sparse-model ../out5/sparsemodel -stack 100 \
      -sparse-model-allow-non-local-wts \
      < data/test_fr.lc |
      ./split-feature-values.pl > out6/test.out) 2>&1 |
      ./grepout-timing.pl > out6/log.canoe.notiming
   diff out6 ref6 -q
   echo "PASS: canoe with full sparsemodel"
}
#testcase3


function testcase4 {
   # Convert the classes files into MemoryMapped_map.
   for f in data/classes.?? data/freq-word-classes.??; do wordClasses2MMmap $f $f.MMmap; done 2> log.wordClasses2MMmap

   [[ -d out7 ]] || mkdir out7
   # Change the text word-classes to their MemoryMapped_map equivalent.
   sed -e 's/\(classes\.\(fr\|en\)\)/\1.MMmap/' data/sparsemodel-big > out7/sparsemodel
   palminer -v -pop -m out7/sparsemodel 2> out7/log.palminer
   diff out7 ref7 -q

   [[ -d out8 ]] || mkdir out8
   (canoe -f data/canoe.ini.cow -sfvals -ffvals -sparse-model ../out7/sparsemodel -stack 100 \
      -sparse-model-allow-non-local-wts \
      < data/test_fr.lc |
      ./split-feature-values.pl > out8/test.out) 2>&1 |
      sed -e 's/\.MMmap//; s/out8/out6/g' |
      ./grepout-timing.pl > out8/log.canoe.notiming
   # This run must be equivalent to ref6 since the only difference is the model type.
   diff out8 ref8 -q
   echo "PASS: canoe with full sparsemodel using MemoryMapped_map."
}
testcase4
