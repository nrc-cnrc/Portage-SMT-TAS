#!/usr/bin/make -f
# vim:noet:ts=3
# Makefile - Unit-test for configtool.
# Tests
#
# PROGRAMMER: Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

#CONFIGTOOL = ~/workspace/PORTAGEshared/src/canoe/configtool
CONFIGTOOL = configtool
SHELL = bash

.SECONDARY:

all: test.check.text.good
all: test.check.tp.good
all: test.check.tp.bad

all:
	@echo All tests PASSED.

LINKS = models/cpt.tppt.ok models/ldm.tpldm.bad models/lm.tplm.bad

test.check.tp.bad: ${LINKS}

models/%:
	-ln -s $(basename $*) models/$*

clean:
	${RM} log.run-test ${LINKS}

test.check.%.good:
	${CONFIGTOOL} check canoe.ini.$*.good 2>&1 \
	| diff -q - <(echo "ok")

test.check.%.bad:
	${CONFIGTOOL} check canoe.ini.$*.bad 2>&1 \
	| diff -q - ref/ref.$*.bad
