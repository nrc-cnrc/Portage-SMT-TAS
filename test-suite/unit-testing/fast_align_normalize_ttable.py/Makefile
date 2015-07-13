#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief Unittest for fast_align_normalize_ttable.py.
# 
# @author Samuel Larkin
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada


SHELL := bash
FAST_ALIGN_NORMALIZE_TTABLE := fast_align_normalize_ttable.py
-include Makefile.params

TEMP_FILES := *.cpt

include ../Makefile.incl

.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


.PHONY:  testsuite


%.check:  %.cpt  ref/%
	diff $+  --brief

# In this testcase we have a value that is less than 1e-30 for source c thus it
# should not be included in the normalization constant nor the output.
testsuite:  testcase1.check
testcase1.cpt:  src/testcase1
	${FAST_ALIGN_NORMALIZE_TTABLE} $< > $@
