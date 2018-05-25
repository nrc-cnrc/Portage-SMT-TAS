#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Testsuite for PortageLiveLib
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada


-include Makefile.params

TEMP_DIRS = plive
include ../Makefile.incl


SHELL = bash

.PHONY: all
all:  testSuite

.PHONY:  testSuite
testSuite: testIncrAddSentence
testSuite: testIncrStatus
testSuite: testTranslateFile

.PHONY:  testIncrAddSentence
testIncrAddSentence:
	./unittest.sh tests/testIncrAddSentence.php

.PHONY:  testIncrStatus
testIncrStatus:
	./unittest.sh tests/testIncrStatus.php

.PHONY:  testTranslateFile
testTranslateFile:
	./unittest.sh tests/testTranslateFile.php

