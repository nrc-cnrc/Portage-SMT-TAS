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
vpath REF.% ref

.PHONY: all
all: full two one

TEMP_FILES=test.*
include ../../Makefile.incl

.PHONY: full two one

full two one:  %:  test.% REF.%
	diff $+ --brief

test.full:  ${COUNT_FILES}
test.two:  afp_eng.tok.4grams apw_eng.tok.4grams
test.one:  afp_eng.tok.4grams

test.%:
	${MERGE_COUNTS} -t $@ $+



################################################################################
# DEBUG
.PHONY: debug
debug: afp_eng.tok.4grams
	${MERGE_COUNTS} -t -d - $+ 2>&1 | less
