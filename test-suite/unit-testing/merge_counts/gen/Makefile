#!/utestsr/bin/make -f
# vim:noet:ts=3:nowrap
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


SHELL = bash

MERGE_COUNTS ?= merge_counts
#MERGE_COUNTS ?= valgrind --leak-check=full merge_counts
DIFF ?= diff -sq


vpath % data

.PHONY: all
all:
	@echo All tests PASSED


.PHONY: clean
clean:
	# Nothing to do


# Targets cannot be PHONY.
empty_file: e
one_file: a

all: empty_file one_file
empty_file one_file: %:
	${MERGE_COUNTS} - $< | ${DIFF} - $<

twice_file: a a

3_file: c b a

4_file: c b a c

one_file_empty_file: a e

big: $(wildcard data/*gram)

all: twice_file 3_file 4_file one_file_empty_file big
twice_file 3_file 4_file one_file_empty_file big: %:
	diff <(${MERGE_COUNTS} - $+) ref/$@ -q

all: unordered
unordered:
	${MERGE_COUNTS} - <(env echo -e "a 1\na 2\nb 4\na 1") 2>&1 \
	| grep Error \
	| grep -q 'is not LC_ALL=C sorted'
