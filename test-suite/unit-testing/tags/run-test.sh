#!/bin/bash
# @file run-test.sh
# @brief Run this test suite
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

set -o errexit
set -o verbose

make clean
make gitignore

STRIP_ENTITY="perl -MULexiTools -ple 'strip_xml_entities(\$_)'"

make -B xtags.tok
make -B xtags.baseline.tok

$STRIP_ENTITY < xtags.tok > xtags.strip
$STRIP_ENTITY < xtags.baseline.tok > xtags.baseline.strip
# gvimdiff xtags.baseline.strip xtags.strip

make -B xtags.baseline.ss.cmp


if which-test.sh chinese_segmenter.pl; then
   ZH_SEG=chinese_segmenter.pl
elif [[ -x ../../../third-party/chinese-segmentation/chinese_segmenter.pl ]]; then
   seg_path=`readlink -f ../../../third-party/chinese-segmentation`
   ZH_SEG="$seg_path/chinese_segmenter.pl -dic $seg_path/manseg.fre"
else
   echo 'Cannot find chinese_segmenter.pl'
   exit 1
fi

make -B zh.tok ZH_SEG="$ZH_SEG"

#utokenize.pl -noss -lang=en -xtags < zh > zh.baseline.tok

$STRIP_ENTITY < zh.tok > zh.strip

$STRIP_ENTITY < zh > zh-notags
$ZH_SEG < zh-notags > zh-notags.tok
# gvimdiff zh-notags.tok zh.strip

make -B zh.ss

set +o verbose
for x in ref/*; do
   echo "diff $x `basename $x` -q"
   diff $x `basename $x` -q
done
set -o verbose

make errors
make trx.diff
make trzh.diff

echo All tests PASSED.
