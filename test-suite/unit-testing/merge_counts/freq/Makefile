#!/usr/bin/make -f
# vim:noet:ts=3:nowrap
#
# @file Makefile run this test suite
# @brief Comparing merge_counts to ngram-merge.
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
MERGE_COUNTS_T ?= ${MERGE_COUNTS} -t

COUNT_FILES ?= $(notdir $(wildcard data/*4grams))

vpath %.4grams data

.PHONY: all
all: full two one
	@echo All tests PASSED


.PHONY: clean
clean:
	# Nothing to do


# Make sure the inputs are LC_ALL=C sort which is a requirement from merge_counts.
.PHONY: validate
validate: $(addsuffix .validate, ${COUNT_FILES})
%.validate: %
	LC_ALL=C sort -c $<


# Cannot be PHONY.
full: ${COUNT_FILES}

# Cannot be PHONY.
two: afp_eng.tok.4grams apw_eng.tok.4grams

# Make sure merge_counts's output is identical to ngream-merge's output.
full two: %: validate
full two: %:
	diff ref/$@ <(${MERGE_COUNTS_T} - $(filter %.4grams, $+)) -q

# Special case since ngram-merge requires at least 2 input files.
.PHONY: one
one: afp_eng.tok.4grams afp_eng.tok.4grams.validate
	${MERGE_COUNTS_T} - $(filter %.4grams, $+) | diff -q - $(filter %.4grams, $+)



################################################################################
# HELPERS
.PHONY: list
list:
	echo "${COUNT_FILES}"
	echo "$(wildcard data/*4grams)"

.PHONY: debug
debug: afp_eng.tok.4grams
	${MERGE_COUNTS_T} -t -d - $+ 2>&1 | less
