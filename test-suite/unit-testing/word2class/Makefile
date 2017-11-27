#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada
# Copyright 2017, Her Majesty in Right of Canada

NNJM_DIR := ${PORTAGE}/test-suite/systems/toy-regress-en2fr.nnjm/models/nnjm/
INPUT := ${PORTAGE}/test-suite/tutorial-data/
WORD2CLASS = word2class

VPATH := ${NNJM_DIR} ${INPUT}

-include Makefile.params

TEMP_FILES = *.mmcls
include ../Makefile.incl


.DEFAULT_GOAL := all
.PHONY: all
all:  testSuite


.PHONY: clean
clean:


.PHONY:  testSuite
testSuite:


.PHONY:  text_testcase
testSuite:  text_testcase
text_testcase:  dev1_en.al  en.400.classes
	${WORD2CLASS} $+ \
	| diff ref/dev1_en.ref.classes -  --brief


%.mmcls:  %.classes
	wordClasses2MMmap $< $@


.PHONY:  mmcls_testcase
testSuite:  mmcls_testcase
mmcls_testcase:  dev1_en.al  en.400.mmcls
	${WORD2CLASS} $+ \
	| diff ref/dev1_en.ref.classes -  --brief
