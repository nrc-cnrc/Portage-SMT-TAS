# vim:noet:ts=3
#
# @file Makefile Run this sub-test suite dealing with bad input and output in merge_count
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada
# Copyright 2012, Her Majesty in Right of Canada

SHELL=/bin/bash

all: bad
all: empty

TEMP_FILES=empty.gz bad-out.gz
include ../../Makefile.incl

# this test still fails, so don't activate it yet.
empty:
	merge_counts empty.gz /dev/null
	(( `wc -c < empty.gz` == 20 ))

bad:
	! { merge_counts bad-out.gz data/bad.gz; }
	merge_counts bad-out.gz data/bad.gz 2>&1 | grep 'File may be corrupt'
	@echo bad test passed since bad.gz is indeed corrupt
