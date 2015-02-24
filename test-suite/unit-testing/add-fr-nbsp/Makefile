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
test-regular: diff-test1 test4 test5 test6

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

# regression testing: this script should not affect the xtags files from the
# tags unit test suite.
test5:
	add-fr-nbsp.pl < ../tags/xtags > xtags.out
	diff ../tags/xtags xtags.out -q

test6:
	cat ../tags/xtags | sed -e 's/<g/<1/g' -e 's/<\/g>/<\/1>/g' > xtags.1
	add-fr-nbsp.pl < xtags.1 > xtags.1.out
	diff xtags.1 xtags.1.out -q

TEMP_FILES=test1 test2 xtags.*
include ../Makefile.incl
