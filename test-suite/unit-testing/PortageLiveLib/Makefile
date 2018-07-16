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

TEMP_DIRS = plive tests/models/fixedTerms
include ../Makefile.incl


SHELL = bash

.PHONY: all
all:  testSuite

.PHONY:  testSuite
php_version := $(shell php --version | grep -o 'PHP [0-9]\.[0-9]\+\.[0-9]\+')
$(info php version: ${php_version})
ifeq ("$(shell [[ "${php_version}" > "PHP 5.4" ]] && echo 0)","0")
testSuite: testIncrAddSentence
testSuite: testIncrStatus
testSuite: testTranslateFile
testSuite: testFixedTerms
else
$(warning Warning: This unittest requires php >=5.4, php>=5.4 could not be found, skipping unittest.)
$(warning Warning: Please install php >=5.4)
endif

.PHONY:  testIncrAddSentence
testIncrAddSentence:
	./unittest.sh tests/testIncrAddSentence.php

.PHONY:  testIncrStatus
testIncrStatus:
	./unittest.sh tests/testIncrStatus.php

.PHONY:  testTranslateFile
testTranslateFile:
	./unittest.sh tests/testTranslateFile.php

.PHONY:  testFixedTerms
testFixedTerms:
	./unittest.sh tests/testFixedTerms.php
