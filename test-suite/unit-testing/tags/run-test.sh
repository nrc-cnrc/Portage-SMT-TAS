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

tok-with-tags-split.pl xtags xtags.text_only xtags.tags
utokenize.pl -noss -lang=en < xtags.text_only > xtags.text_tok
tok-with-tags-combine.pl xtags.text_tok xtags.tags xtags.out

utokenize.pl -noss -lang=en -xtags < xtags > xtags.baseline.tok

$STRIP_ENTITY < xtags.out > xtags.strip
$STRIP_ENTITY < xtags.baseline.tok > xtags.baseline.strip
# gvimdiff xtags.baseline.strip xtags.strip

if which-test.sh chinese_segmenter.pl; then
   ZH_SEG=chinese_segmenter.pl
elif [[ -x ../../../third-party/chinese-segmentation/chinese_segmenter.pl ]]; then
   seg_path=`readlink -f ../../../third-party/chinese-segmentation`
   ZH_SEG="$seg_path/chinese_segmenter.pl -dic $seg_path/manseg.fre"
else
   echo 'Cannot find chinese_segmenter.pl'
   exit 1
fi

tok-with-tags-split.pl zh zh.text_only zh.tags
$ZH_SEG < zh.text_only > zh.text_tok
tok-with-tags-combine.pl zh.text_tok zh.tags zh.out

#utokenize.pl -noss -lang=en -xtags < zh > zh.baseline.tok

$STRIP_ENTITY < zh.out > zh.strip

$STRIP_ENTITY < zh > zh-notags
$ZH_SEG < zh-notags > zh-notags.tok
# gvimdiff zh-notags.tok zh.strip

for x in ref/*; do
   echo "diff $x `basename $x`"
   diff $x `basename $x`
done

echo All tests PASSED.
