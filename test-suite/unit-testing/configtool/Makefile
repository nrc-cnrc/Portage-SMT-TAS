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
all: test.unal.good test.unal.bad
all: all-commands.canoe.ini.text.good
all: all-commands.canoe.ini.tp.good
all: all-commands.canoe.ini.both
all: all-commands.canoe.ini.test.unal.good

LINKS = models/cpt.tppt.ok models/ldm.tpldm.bad models/lm.tplm.bad

test.check.tp.bad: ${LINKS}

# Copy the models instead of linking to avoid an Eclipse bug that resulted
# in the removal of the CVS directories from the original model directories.
models/%:
	cp -r models/$(basename $*) models/$* && ${RM} -r models/$*/CVS

TEMP_FILES=log.run-test canoe.ini.test.unal.good canoe.ini.test.unal.bad
TEMP_DIRS=${LINKS}
include ../Makefile.incl

test.check.%.good:
	${CONFIGTOOL} check canoe.ini.$*.good 2>&1 \
	| diff -q - <(echo "ok")

test.check.%.bad:
	${CONFIGTOOL} check canoe.ini.$*.bad 2>&1 \
	| diff -q - ref/ref.$*.bad

canoe.ini.test.unal.good: test.unal.good

test.unal.good:
	cat canoe.ini.text.good > canoe.ini.$@
	echo '[unal-feature] any:edges:tgtleft:srcright' >> canoe.ini.$@
	${CONFIGTOOL} check canoe.ini.$@

test.unal.bad:
	cat canoe.ini.text.good > canoe.ini.$@
	echo '[unal-feature] any:edges:shmoo:faz' >> canoe.ini.$@
	if ${CONFIGTOOL} check canoe.ini.$@; then false; else true; fi

all-commands.%: %
	for command in weights nf nb nl nt na nt-tppt nd segff ttable-limit \
	      memmap check tp list-lm list-ldm list-tm get-all-wts:x \
	      rescore-model:ffvals; do \
	   echo ======================= $$command; \
	   configtool $$command $*; \
	done | diff - ref/$@
