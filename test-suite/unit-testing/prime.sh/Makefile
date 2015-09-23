#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief Validates that prime.sh for a protagelive system is functional.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

SYSTEM := ${PORTAGE}/test-suite/systems/toy-regress-en2fr


-include Makefile.params
-include ../Makefile.incl

SHELL := bash

.DEFAULT_GOAL := all
.PHONY: all
all:  testSuite


.PHONY:  testSuite
testSuite:  prime.partial
testSuite:  prime.full
testSuite:  prime.bad
testSuite:  translate


.PHONY: clean
clean:


# prime.sh is only valid with partial & full anything else is a wrong prime mode.
prime.bad:  prime.%:
	! ${SYSTEM}/prime.sh $*

prime.%:
	${SYSTEM}/prime.sh $*


# There is a problem when translating an empty source with a system that uses
# confidence estimation.  gen_feature_values doesn't like empty inputs.
translate:
	${SYSTEM}/soap-translate.sh -with-ce <<< '' &> /dev/null
