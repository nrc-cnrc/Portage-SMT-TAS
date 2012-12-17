#!/utestsr/bin/make -f
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
DIFF ?= diff -sq


.PHONY: all
all: empty_file one_file twice_file 3_file 4_file one_file_empty_file big
	echo All tests PASSED.


.PHONY: one_file
empty_file:
	${MERGE_COUNTS} - data/e | ${DIFF} - data/e

.PHONY: one_file
one_file:
	${MERGE_COUNTS} - data/a | ${DIFF} - data/a

.PHONY: twice_file
twice_file:
	${MERGE_COUNTS} - data/a data/a | ${DIFF} - data/a2

.PHONY: 3_file
3_file:
	${MERGE_COUNTS} - data/a data/b data/c | ${DIFF} - data/abc

.PHONY: 4_file
4_file:
	${MERGE_COUNTS} - data/a data/b data/c data/c | ${DIFF} - data/abc2

.PHONY: one_file_empty_file
one_file_empty_file:
	${MERGE_COUNTS} - data/a data/e | ${DIFF} - data/a

.PHONY: big
big:
	${MERGE_COUNTS} - data/*gram | ${DIFF} - data/big

.PHONY: clean
clean:
	# Nothing to do
