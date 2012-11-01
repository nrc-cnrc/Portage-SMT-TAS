#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


-include Makefile.params

.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


TEMP_FILES=sorted.* unsorted.*
include ../Makefile.incl


.PHONY:  testsuite
testsuite:  order


.PHONY:  order
order:  test.sorted.en2fr
order:  test.unsorted.en2fr
order:  test.sorted.en2fr.gz
order:  test.unsorted.en2fr.gz
order:  compare.sorted
order:  compare.unsorted

test.sorted.%:  sorted.%
	zcat -f $< | LC_ALL=C sort --check

test.unsorted.%:  unsorted.%
	! { zcat -f $< | LC_ALL=C sort --check; }

compare.%:  %.en2fr  %.en2fr.gz
	zcmp  $+

unsorted.en2fr:  SORT=-no-sort
unsorted.en2fr sorted.en2fr:  %.en2fr:
	joint2multi_cpt \
		${SORT} \
		-prune1w 100 \
		-v -i \
		-1 en -2 fr \
		-s '0:KNSmoother 3' -a '1-100:PureIndicator' \
		-dir fwd \
		-o $* \
		src/ibm2 src/hmm3 \
		&> $@.log
	[[ `find -name $@ -size +0c | \wc -l` -eq 1 ]]

unsorted.en2fr.gz:  SORT=-no-sort
unsorted.en2fr.gz sorted.en2fr.gz:  %.en2fr.gz:
	joint2multi_cpt \
		${SORT} \
		-prune1w 100 \
		-v -i -z \
		-1 en -2 fr \
		-s '0:KNSmoother 3' -a '1-100:PureIndicator' \
		-dir fwd \
		-o $* \
		src/ibm2 src/hmm3 \
		&> $@.log
	[[ `find -name $@ -size +21c | \wc -l` -eq 1 ]]

