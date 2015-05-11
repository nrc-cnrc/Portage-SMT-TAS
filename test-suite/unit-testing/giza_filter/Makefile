#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief TestSuite for giza_filter
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


GIZA_FILTER := giza_filter

-include Makefile.params

vpath giza_filter.% src

SHELL := bash
.DEFAULT_GOAL := all
.PHONY: all
all:  testSuite


TEMP_FILES=unittest*
include ../Makefile.incl


.PHONY:  testSuite
testSuite:  unittest1
testSuite:  unittest2
testSuite:  unittest3
testSuite:  unittest4
testSuite:  unittest5
testSuite:  unittest6


# Without ids.
.PHONY:  unittest1
unittest1:  %:  %.src %.tgt ref/giza_filter.src ref/giza_filter.tgt
	diff --brief $(filter %.src, $+)
	diff --brief $(filter %.tgt, $+)

unittest1%src unittest1%tgt:  giza_filter.src giza_filter.tgt
	${GIZA_FILTER} $+ unittest1$*src unittest1$*tgt 2>&1 | grep --quiet '6 sentence pairs were dropped.'



# With ids.
.PHONY:  unittest2
unittest2:  %:  %.src %.tgt %.id ref/giza_filter.src ref/giza_filter.tgt ref/giza_filter.id
	diff --brief $(filter %.src, $+)
	diff --brief $(filter %.tgt, $+)
	diff --brief $(filter %.id, $+)

unittest2%src unittest2%tgt unittest2%id:  giza_filter.src giza_filter.tgt giza_filter.id
	${GIZA_FILTER} \
		-iid $(filter %.id, $+) \
		-oid unittest2$*id \
		$(filter-out %.id, $+) unittest2$*src unittest2$*tgt 2>&1 | grep --quiet '6 sentence pairs were dropped.'


# Too many ids.
.PHONY:  unittest3
unittest3:  giza_filter.src giza_filter.tgt giza_filter.id
	${GIZA_FILTER} \
		-oid /dev/null \
		-iid <(cat $(filter %.id,$+) $(filter %.id,$+)) \
		$(filter %.src,$+) \
		$(filter %.tgt,$+) \
		/dev/null /dev/null \
		2>&1 | grep --quiet 'There is still some id.'

# Too many source.
.PHONY:  unittest4
unittest4:  giza_filter.src giza_filter.tgt giza_filter.id
	${GIZA_FILTER} \
		-oid /dev/null \
		-iid $(filter %.id,$+) \
		<(cat $(filter %.src,$+) $(filter %.src,$+)) \
		$(filter %.tgt,$+) \
		/dev/null /dev/null \
		2>&1 | grep --quiet 'There is still some source.'

# Too many target.
.PHONY:  unittest5
unittest5:  giza_filter.src giza_filter.tgt giza_filter.id
	${GIZA_FILTER} \
		-oid /dev/null \
		-iid $(filter %.id,$+) \
		$(filter %.src,$+) \
		<(cat $(filter %.tgt,$+) $(filter %.tgt,$+)) \
		/dev/null /dev/null \
		2>&1 | grep --quiet 'There is still some target.'

# Missing oid file.
.PHONY:  unittest6
unittest6:  giza_filter.src giza_filter.tgt giza_filter.id
	${GIZA_FILTER} \
		-iid $(filter %.id,$+) \
		$(filter %.src,$+) \
		$(filter %.tgt,$+) \
		/dev/null /dev/null \
		2>&1 | grep --quiet "Error: You've provided an input file for ids but neglected to provide the output file for ids."

