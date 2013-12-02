#!/bin/bash
# @file Makefile
# @brief Rules for actually doing stuff in this test suite.
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

TEMP_FILES=zh.* xtags.* zh-notags*
include ../Makefile.incl

SHELL=/bin/bash

all: xtags.tok zh.tok
all: xtags.ss
all: xtags.baseline.ss.cmp

errors: errors.tok errors.ss

xtags.tok: xtags
	tok-with-tags-split.pl xtags xtags.text_only xtags.tags
	utokenize.pl -noss -lang=en < xtags.text_only > xtags.text_tok
	tok-with-tags-combine.pl xtags.text_tok xtags.tags $@

xtags.baseline.tok: xtags
	utokenize.pl -noss -lang=en -xtags < $< | sed 's/ $$//' > $@

# This is not actually an appropriate test - compare to ref/xtags.tok instead
xtags.baseline.tok.cmp: xtags.tok xtags.baseline.tok
	diff $+ -q

errors.tok: xtags.tok
	! tok-with-tags-combine.pl <(head xtags.text_tok) xtags.tags /dev/null
	! tok-with-tags-combine.pl xtags.text_tok <(head xtags.tags) /dev/null

zh.tok: zh
	tok-with-tags-split.pl zh zh.text_only zh.tags
	${ZH_SEG} < zh.text_only > zh.text_tok
	tok-with-tags-combine.pl zh.text_tok zh.tags $@

xtags.ss: xtags
	sentsplit-with-tags-split.pl xtags xtags.ss-text xtags.ss-tags
	utokenize.pl -paraline -ss -notok -lang=en < xtags.ss-text > xtags.ss-text.ss
	sentsplit-with-tags-combine.pl xtags.ss-text.ss xtags.ss-tags $@

xtags.baseline.ss: xtags
	utokenize.pl -ss -paraline -notok -lang=en -xtags < $< > $@

xtags.baseline.ss.cmp: xtags.ss xtags.baseline.ss
	diff $+ -q

zh.ss: zh
	sentsplit-with-tags-split.pl zh zh.ss-text zh.ss-tags
	sentsplit_plugin ch < zh.ss-text > zh.ss-text.ss
	sentsplit-with-tags-combine.pl zh.ss-text.ss zh.ss-tags $@

errors.ss: xtags.ss
	! sentsplit-with-tags-combine.pl xtags.ss-text.ss <(head xtags.ss-tags) /dev/null
	! sentsplit-with-tags-combine.pl <(head xtags.ss-text.ss) xtags.ss-tags /dev/null


