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
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

PARSE_TOKAN := parse_tokan.pl

-include Makefile.params

include ../Makefile.incl

vpath input% src

.DEFAULT_GOAL := all


.PHONY:  6fields.test
all:  6fields.test
6fields.test:  input.6fields ref/input.6fields
	${PARSE_TOKAN} 6 1 < $< \
	| diff - $(filter ref%, $+) --brief


.PHONY:  1fields.test
all:  1fields.test
1fields.test:  input.1field ref/input.1field
	${PARSE_TOKAN} 1 0 < $< \
	| diff - $(filter ref%, $+) --brief
