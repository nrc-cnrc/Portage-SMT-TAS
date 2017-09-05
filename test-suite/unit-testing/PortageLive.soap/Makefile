#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Testsuite for PortageLiveLib
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada


-include Makefile.params

TEMP_DIRS=doc_root scenarios
include ../Makefile.incl


SHELL = bash

.PHONY: all
all:  testSuite

.PHONY:  testSuite
testSuite:
	./unittest.sh
