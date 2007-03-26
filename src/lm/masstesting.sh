#!/bin/bash

# Compare the output of our LM implementation with that of SRILM on identical
# input.
# Usage: masstesting.sh lm_for_srilm lm_for_portage testset

# example test from the test suite, using 96 input lines:
# masstesting.sh $PORTAGE/test-suite/regress-small-voc/europarl.en.srilm{,} $PORTAGE/test-suite/regress-small-voc/lc/test2000.en.lowercase
# Bigger example test, using 55813 input lines:
# masstesting.sh $PORTAGE/test-suite/regress-small-voc/europarl.en.srilm{,} $PORTAGE/test-suite/regress-small-voc/lc/europarl.fr-en.en.lowercase

echo 'masstesting.sh, NRC-CNRC, Copyright (c) 2006 - 2007, Her Majesty in Right of Canada'

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
echo "=================== LMText -limit =================="
echo ""
echo time ./testlm -limit $LM_FOR_PORTAGE $TEST \> lmtexttestfile
echo ""
time ./testlm -limit $LM_FOR_PORTAGE $TEST > lmtexttestfile

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
echo time ./testlm $LM_FOR_PORTAGE $TEST \> lmtexttestfile-no-limit
echo ""
time ./testlm $LM_FOR_PORTAGE $TEST > lmtexttestfile-no-limit

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
   echo "Something went wrong! One or many tests are not accurate!"
else
   echo "OK! All tests look allright!"
fi

echo ""
