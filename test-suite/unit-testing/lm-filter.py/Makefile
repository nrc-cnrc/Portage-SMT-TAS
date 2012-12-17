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


LM_FILTER := lm-filter.py

-include Makefile.params

SHELL = bash

vpath %.gz src/

.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


TEMP_FILES=unittest*.out
include ../Makefile.incl


.PHONY: testsuite
testsuite:  unittest1
testsuite:  unittest2
testsuite:  unittest3



########################################
# Checks if the thresholds are in ascending order.
.PHONY: unittest1
unittest1:  unittest1.out
	grep -q 'Thresholds must be ascending in value' $<
	@echo Bad ordering test PASSED.

unittest1.out:  lm.gz counts.gz 
	! ${LM_FILTER} $+ 2 1 &> $@



########################################
# Filters the LM.
# Missing threshold for 3-gram should be deduce from the threshold for 2-gram.
.PHONY: unittest2
unittest2:  unittest2.out
	grep -q 'Using a minimum of 2 counts to prune 2-gram' $<
	grep -q 'Using a minimum of 2 counts to prune 3-gram' $<
	grep -q 'ngram 1=7' $<
	grep -q 'ngram 2=2' $<
	grep -q 'ngram 3=1' $<
	@echo Missing threshold test PASSED.

unittest2.out:  lm.gz counts.gz 
	${LM_FILTER} $+ 1 2 &> $@



########################################
# Invalid count file format.
.PHONY: unittest3
unittest3:  unittest3.out
	grep -q "Fatal error: Invalid format of counts in .*: \[u'E   3'\]" $+
	@echo Invalid count file format test PASSED.

unittest3.out:  lm.gz counts.gz
	! ${LM_FILTER} $(word 1, $+) <(zcat -f $(word 2, $+) | sed -e 's/^E	3$$/E   3/') 0 2 &> $@
