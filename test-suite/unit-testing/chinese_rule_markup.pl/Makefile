#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada


CHINESE_RULE_MARKUP = chinese_rule_markup.pl

-include Makefile.params

vpath input src/
vpath %.marked ref/

TEMP_FILES := double.percent.marked
include ../Makefile.incl

.DEFAULT_GOAL := all
.PHONY: all
all:  double.percent.test


.PHONY: clean
clean:

# This test was triggered by the fact that Boxing, during BOLT 2014, came
# across a problem where  百 百分之 八十 which literally mean "100 100 percent
# 80" or closet to English "100 80%".  The error was showing up when two
# consecutive tokens were supposed to be marked.
double.percent.marked:  input
	${CHINESE_RULE_MARKUP} < $< > $@

double.percent.test:  double.percent.marked  input.marked
	diff $+ --brief
