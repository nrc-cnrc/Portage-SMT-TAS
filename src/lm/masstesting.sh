#!/bin/bash

# Compare the output of our LM implementation with that of SRILM on identical
# input.
# Usage: masstesting.sh lm_for_srilm lm_for_portage testset

# example test from the test suite, using 96 input lines:
# masstesting.sh $PORTAGE/test-suite/regress-small-voc/europarl.en.srilm{,} $PORTAGE/test-suite/regress-small-voc/lc/test2000.en.lowercase
# Bigger example test, using 55813 input lines:
# masstesting.sh $PORTAGE/test-suite/regress-small-voc/europarl.en.srilm{,} $PORTAGE/test-suite/regress-small-voc/lc/europarl.fr-en.en.lowercase

# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

# Include NRC's bash library.
BIN=`dirname $0`
if [[ ! -r $BIN/sh_utils.sh ]]; then
   # assume executing from src/* directory
   BIN="$BIN/../utils"
fi
source $BIN/sh_utils.sh

print_nrc_copyright masstesting.sh 2006
export PORTAGE_INTERNAL_CALL=1

if [[ $# -ne 3 ]]; then
   error_exit "Your are missing some arguments"
fi

LM_FOR_SRILM=$1
LM_FOR_PORTAGE=$2
TEST=$3

#we first do the ngram SRILM way of doing things, egrep logs and put that in a file

echo time ngram -lm $LM_FOR_SRILM -ppl $TEST -debug 2 '| egrep -e"^	p" |  sed "s/.*] //" |' " cut -f3 -d' ' > srilmtestfile"
time ngram -lm $LM_FOR_SRILM -ppl $TEST -debug 2 2 | egrep -e"^	p" |  sed "s/.*] //" | cut -f3 -d' ' > srilmtestfile

# then we do the same thing with our various implementations and compare the
# results

ALL_GOOD=1

echo ""
echo "=================== LMText -limit -per-sent-limit =================="
echo ""
echo time ./lm_eval -limit -per-sent-limit $LM_FOR_PORTAGE $TEST \> lmtexttestfile-per-sent
echo ""
time ./lm_eval -limit -per-sent-limit $LM_FOR_PORTAGE $TEST > lmtexttestfile-per-sent

echo ""
echo diff -q srilmtestfile lmtexttestfile-per-sent
if ! diff -q srilmtestfile lmtexttestfile-per-sent; then
   echo 'Differences found between SRILM and LMText with -limit and -per-sent-limit.'
   echo Of those, `diff-round.pl srilmtestfile lmtexttestfile-per-sent 2> /dev/null | wc -l` are not just rounding errors
   diff-round.pl srilmtestfile lmtexttestfile-per-sent | head
   ALL_GOOD=0
fi


echo ""
echo "=================== LMText -limit =================="
echo ""
echo time ./lm_eval -limit $LM_FOR_PORTAGE $TEST \> lmtexttestfile
echo ""
time ./lm_eval -limit $LM_FOR_PORTAGE $TEST > lmtexttestfile

echo ""
echo diff -q srilmtestfile lmtexttestfile
if ! diff -q srilmtestfile lmtexttestfile; then
   echo 'Differences found between SRILM and LMText with -limit.'
   echo Of those, `diff-round.pl srilmtestfile lmtexttestfile 2> /dev/null | wc -l` are not just rounding errors
   diff-round.pl srilmtestfile lmtexttestfile | head
   ALL_GOOD=0
fi


echo ""
echo "====================== LMText ======================"
echo ""
echo time ./lm_eval $LM_FOR_PORTAGE $TEST \> lmtexttestfile-no-limit
echo ""
time ./lm_eval $LM_FOR_PORTAGE $TEST > lmtexttestfile-no-limit

echo ""
echo diff -q srilmtestfile lmtexttestfile-no-limit
if ! diff -q srilmtestfile lmtexttestfile-no-limit; then
   echo 'Differences found between SRILM and LMText without -limit.'
   echo Of those, `diff-round.pl srilmtestfile lmtexttestfile-no-limit 2> /dev/null | wc -l` are not just rounding errors
   diff-round.pl srilmtestfile lmtexttestfile-no-limit | head
   ALL_GOOD=0
fi


echo ""
echo "====================== Summary ====================="
echo ""

if [ $ALL_GOOD = 0 ]; then
   echo "Something went wrong! One or more tests are not accurate!"
else
   echo "OK! All tests look allright!"
fi

echo ""
