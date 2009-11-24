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

COUNT_FILES ?= aa.jpt.gz  ab.jpt.gz  ac.jpt.gz  ad.jpt.gz  ae.jpt.gz
vpath a%.jpt.gz data

.PHONY: all
all: full two one
	@echo All tests PASSED

clean:
	${RM} test.* REF.*

REF.full: ${COUNT_FILES}
	zcat $+ | joint2cond_phrase_tables -ij | LC_ALL=C sort > $@
	
.PHONY: full
full: ${COUNT_FILES} REF.full
	${MERGE_COUNTS} test.$@ $(filter %.jpt.gz,$+)
	diff -q test.$@ REF.$@

.PHONY: two
two: aa.jpt.gz ab.jpt.gz
	zcat $+ | joint2cond_phrase_tables -ij | LC_ALL=C sort > REF.$@
	${MERGE_COUNTS} test.$@ $+
	diff -q test.$@ REF.$@

.PHONY: one
one: aa.jpt.gz
	${MERGE_COUNTS} test.$@ $+
	zdiff -q test.$@ $+

.PHONY: debug
debug: aa.jpt.gz
	${MERGE_COUNTS} -d - $+ 2>&1 | less
