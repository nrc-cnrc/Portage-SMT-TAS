#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief Validates that canoe can output its nnsi to json.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

CANOE := canoe
JQ := jq

-include Makefile.params

TEMP_FILES := nssi.json

include ../Makefile.incl

vpath canoe.ini.cow  ${PORTAGE}/test-suite/systems/toy-regress-en2fr/

.DEFAULT_GOAL := all
.PHONY: all
all:  unittest


.PHONY: clean
clean:


.PHONY:  unittest
unittest:  nssi.json.parsable
unittest:  nssi.json.check

nssi.json:  canoe.ini.cow
	echo -e 'This is a test .\nApples cannot be " purple " .' |\
	${CANOE} -f $< -nssiFilename $@

nssi.json.parsable:  %.parsable:  %
	${JQ} . $< &> /dev/null

nssi.json.check:  %.check:  %  ref/%
	diff-round.pl -p 5 'jq . nssi.json | sed "s/,/ ,/" |' 'jq . ref/nssi.json | sed "s/,/ ,/" |'
