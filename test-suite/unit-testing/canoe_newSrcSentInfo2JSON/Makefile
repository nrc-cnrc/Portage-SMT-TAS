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

ifneq ($(shell which jq &> /dev/null && echo 0),0)
#JSON_PP := python -c "import sys, json; [sys.stdout.write(json.dumps(json.loads(d.strip()), indent=2, sort_keys=True)) for d in sys.stdin]"
#JSON_PP := python -c "import sys, json; map(lambda d: sys.stdout.write(json.dumps(json.loads(d.strip()), indent=2, sort_keys=True)), sys.stdin)"
# jq's alternative does not work.
$(error Using jq's alternative "${JSON_PP}")
else
#JSON_PP := jq --sort-keys .
JSON_PP := jq --sort-keys ".potential_phrases[]|= [.[] | sort_by(.phrase)]"
$(info Using jq "${JSON_PP}")
endif



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

# Let's make sure the output is valid json.
nssi.json.parsable:  %.parsable:  %
	${JSON_PP} < $< &> /dev/null

nssi.json.check:  %.check:  %  ref/%
	diff-round.pl \
		-p 5 \
		'${JSON_PP} < nssi.json | sed "s/,/ ,/" |' \
		'${JSON_PP} < ref/nssi.json | sed "s/,/ ,/" |'
