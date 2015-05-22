#!/usr/bin/make -f
# vim:noet:ts=3:nowrap

# @file Makefile
# @brief Unittest that compares textldm tpldm using canoe.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada
 

CANOE ?= canoe
V ?= 2
SHELL := /bin/bash

.PHONY: all
all: testsuite

.PHONY: testsuite
testsuite: unittest twodmtest

TEMP_FILES=vocab output.* log.output.* twodmtest.*
TEMP_DIRS=dm.hmm1+ibm2.en2fr.tpldm dm.hmm1+ibm2.en2fr.tpldm.tmp.*
include ../Makefile.incl



# Translate using text and tpt lexicalized distortion models and make sure the translations are de same.
.PHONY: unittest
unittest: output.tpt
unittest: output.text
unittest:
	diff -q $+



# Translate depending on the canoe.ini.
output.%: input vocab
	${CANOE} \
		-v $V \
		-Voc-file vocab \
		-f canoe.ini.$* \
		< $< \
		> $@ \
		2> log.$@



# For ease of debugging, let's specify a vocab to canoe.
vocab:
	zcat -f cpt.*.gz \
	| perl -nle 'print join " ", (split(/ \|\|\| /))[0,1]' \
	| get_voc \
	> vocab


output.tpt: dm.hmm1+ibm2.en2fr.tpldm
dm.hmm1+ibm2.en2fr.tpldm: dm.hmm1+ibm2.en2fr.gz
	textldm2tpldm.sh $<


# check_ldm_syntax makes sure we can specify (H)LDMs in their wide variety of
# syntaxes. We don't care about the output, just that the command finishes with
# 0 exit status.
.PHONY: check_ldm_syntax
check_ldm_syntax:
	canoe -f canoe.ini.text -distortion-model 'back-lex:fwd-lex:back-lex#0:fwd-lex#0' -input <(echo foo) -weight-d "" >& log.output.$@


# Mini test to make sure using two DMs associates the bkoff correctly with each DM.
.PHONY: twodmtest
twodmtest:
	echo m s d x | canoe -f canoe.ini.2dm -trace -ffvals > $@.out 2> $@.log
	diff $@.out ref/$@.out
