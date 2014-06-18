#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

TOKENIZE_PLUGIN ?= tokenize_plugin_ar
SENTSPLIT_PLUGIN ?= sentsplit_plugin

-include Makefile.params

vpath %_ar  src/

SHELL := bash
.DEFAULT_GOAL := all
.PHONY: all
all:  empty.validate
all:  empty_line.validate
all:  underscore.validate
all:  multiSentence.validate


TEMP_FILES = *.tok

include ../Makefile.incl


# Let's tokenize an input that contains nothing not even a newline.
.PHONY:  empty.validate
empty.validate:  empty.tok
	[[ ! -s $< ]]
empty.tok:
	echo -n "" | ${TOKENIZE_PLUGIN} ar > $@

# Let's tokenize an empty line.
.PHONY:  empty_line.validate
empty_line.validate:  empty_line.tok
	[[ `\wc -l < $<` -eq 1 ]]  # There is oneline
	[[ `grep -c '^$$' < $<` -eq 1 ]]  # There is exactly one empty line
empty_line.tok:
	echo "" | ${TOKENIZE_PLUGIN} ar > $@

# During development, Arabic underscore were causing problems.
.PHONY:  underscore.validate
underscore.validate:  underscore.tok
	[[ `\wc -l < $<` -eq 1 ]]
underscore.tok:
	${TOKENIZE_PLUGIN} ar <<< "ـــــــــــــــــــــــ"  > $@

# This is the original input that was failing where a multisentence line was
# passed to ${SENTSPLIT_PLUGIN} which added an empty line at the end of the
# document then ${TOKENIZE_PLUGIN} was dying on that empty line.
.PHONY:  multisentence.validate
multiSentence.validate:  multiSentence.tok
	[[ `\wc -l < $<` -eq 12 ]]
multiSentence.tok:  Fan_ar
	${SENTSPLIT_PLUGIN} ar < $< | ${TOKENIZE_PLUGIN} ar > $@

