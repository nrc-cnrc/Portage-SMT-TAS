#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# $Id$

# @file Makefile
# @brief Testsuite for compile_truecase_map.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologiesm
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada



-include Makefile.params

# Disable copyright notice.
export PORTAGE_INTERNAL_CALL=1

SHELL = bash

.PHONY: all
all: testsuite


.PHONY: clean
clean:
	${RM} lowercase* truecase*

.PRECIOUS: lowercase
lowercase: compile_truecase_map.unittest
	utf8_casemap -c l < $< > $@

.PRECIOUS: lowercase_remove
lowercase_remove: compile_truecase_map.unittest
	sed -e 's/REMOVE//' < $< | utf8_casemap -c l > $@

.PRECIOUS: truecase_short lowercase_short
truecase_short: compile_truecase_map.unittest
lowercase_short: lowercase
%_short:
	head -3 < $< > $@


# Make sure the map is valid.
testsuite: validate_output
validate_output: compile_truecase_map.unittest lowercase
	compile_truecase_map $+ | LC_ALL=C sort | diff -q - <(sort compile_truecase_map.unittest.ref)

# Make sure with valid inputs, compile_truecase_map succeeds.
testsuite: all_valid_inputs
all_valid_inputs: compile_truecase_map.unittest lowercase
	compile_truecase_map $+ > /dev/null

# Test that compile_truecase_map detects inputs that contain sentence of
# different length when sentences in a aligned pair should be of equal length.
testsuite: uneven_lowercase_truecase
uneven_lowercase_truecase: compile_truecase_map.unittest lowercase_remove
	[[ `compile_truecase_map $+ 2>&1` == "Error: line length mismatch at line 4 in compile_truecase_map.unittest" ]]

# Test to detect that the truecase input is shorter than the lowercase input.
testsuite: shorter_truecase
shorter_truecase: truecase_short lowercase
	[[ `compile_truecase_map $+ 2>&1` =~ "Error: truecase file $< too short" ]]

# Test to detect that the lowercase input is shorter than the truecase input.
testsuite: shorter_lowercase
shorter_lowercase: compile_truecase_map.unittest lowercase_short
	[[ `compile_truecase_map $+ 2>&1` =~ "Error: lowercase file $(filter %_short, $+) too short" ]]

# Test to detect that the truecase input is shorter than the lowercase input.
testsuite: shorter_truecase_2
shorter_truecase_2: compile_truecase_map.unittest truecase_short lowercase lowercase
	[[ `compile_truecase_map $+ 2>&1` =~ "Error: truecase file $(filter %_short, $+) too short" ]]

