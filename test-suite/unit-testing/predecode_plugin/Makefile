#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief Test predecode_pluign
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada
#

FIXED_TERM2TM := fixed_term2tm.pl
PREDECODE_PLUGIN := predecode_plugin
MARK_ENGLISH_NUMBER := mark-english-number.pl

-include Makefile.params

export PORTAGE_INTERNAL_CALL=1
export FIXED_TERMS_MODEL_DIRECTORY=.

TEMP_DIRS = fixedTerms/
include ../Makefile.incl

SHELL := bash
.DEFAULT_GOAL := all
.PHONY: all
all:  mark


.PHONY: clean
clean:


fixedTerms:
	mkdir -p $@

vpath fiches_termino.utf8.txt src/
fixedTerms/tm:  fiches_termino.utf8.txt  | fixedTerms
	${FIXED_TERM2TM} -source=en -target=fr $< | sort --unique > $@

.PHONY:  mark  pre  pre.conv  conv
mark:  pre  pre.conv  conv

pre:  %:  ref/%  fixedTerms/tm
	set -o pipefail; \
	cat src/input \
	| utokenize.pl -noss -lang=en \
	| utf8_casemap -c l \
	| ${PREDECODE_PLUGIN} en \
	| diff - $<

pre.conv:  %:  ref/%  fixedTerms/tm
	set -o pipefail; \
	cat src/input \
	| utokenize.pl -noss -lang=en \
	| utf8_casemap -c l \
	| ${PREDECODE_PLUGIN} en \
	| ${MARK_ENGLISH_NUMBER} en \
	| diff - $<

conv:  %:  ref/%  fixedTerms/tm
	set -o pipefail; \
	cat src/input \
	| utokenize.pl -noss -lang=en \
	| utf8_casemap -c l \
	| canoe-escapes.pl -add | utf8_filter \
	| ${MARK_ENGLISH_NUMBER} en \
	| diff - $<
