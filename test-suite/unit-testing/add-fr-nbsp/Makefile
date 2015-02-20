#!/usr/bin/make -f
# vim:ts=3:noet

# Makefile - unit testing for add-fr-nbsp.pl
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

all: test

test: test-regular
test-regular: diff-test1 test4

# These two test cases only pass if the non-breaking space code is uncommented
# (and thereby activated) in postprocess_plugin:
#test: test-only-when-activated
test-only-when-activated: diff-test2 test3

diff-%: %
	diff $* ref/$* -q

test1: currency.txt
	add-fr-nbsp.pl < $< > $@

test2: currency.txt
	postprocess_plugin fr < $< > $@

test3: test1 test2
	diff test1 test2 -q

test4: currency.txt
	postprocess_plugin en < $< | diff - $< -q

TEMP_FILES=test1 test2
include ../Makefile.incl
