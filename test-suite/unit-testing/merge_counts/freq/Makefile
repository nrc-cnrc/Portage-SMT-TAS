#!/usr/bin/make -f
# vim:noet:ts=3
#
# @file Makefile run this test suite
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

MERGE_COUNTS ?= merge_counts

COUNT_FILES ?= $(subst data/,,$(wildcard data/*4grams))
vpath %.4grams data

.PHONY: all
all: full two one
	@echo All tests PASSED

TEMP_FILES=test.* REF.*
include ../../Makefile.incl

.PHONY: full
full: ${COUNT_FILES}
	ngram-merge -write REF.$@ $+
	${MERGE_COUNTS} -t test.$@ $+
	diff -q test.$@ REF.$@

.PHONY: two
two: afp_eng.tok.4grams apw_eng.tok.4grams
	ngram-merge -write REF.$@ $+
	${MERGE_COUNTS} -t test.$@ $+
	diff -q test.$@ REF.$@

.PHONY: one
one: afp_eng.tok.4grams
	${MERGE_COUNTS} -t test.$@ $+
	diff -q test.$@ $+

.PHONY: debug
debug: afp_eng.tok.4grams
	${MERGE_COUNTS} -t -d - $+ 2>&1 | less
