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
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada



GEN_FEATURE_VALUES := gen_feature_values

-include Makefile.params

SHELL := bash

TEMP_FILES := src nbest both_empty

include ../Makefile.incl

.DEFAULT_GOAL := all
.PHONY: all
all:  testSuite

.PHONY:  testSuite
testSuite:  valid_inputs
testSuite:  both_empty
testSuite:  empty_source
testSuite:  empty_nbest
testSuite:  invalid_nbest_size



# mocked source file
src:
	seq 1 5 > $@

# mocked nbest list file
nbest:
	seq 1 25 > $@



# gen_feature_values should create 25 feature values given a nbest list's size
# of 25.
valid_inputs:  src nbest
	[[ `${GEN_FEATURE_VALUES} LengthFF 0 $+ | wc --lines` -eq 25 ]]

# gen_feature_values should produce and empty feature file given an empty
# source and an empty nbest.
both_empty:
	${GEN_FEATURE_VALUES} -o $@ LengthFF 0 <(echo -n) <(echo -n) 2>&1 \
	| grep 'Warning: empty input files:' --quiet
	[[ -f $@ ]]
	[[ `wc --lines < $@` -eq 0 ]]

# gen_feature_values should report an error if it is given an empty source with
# a none empty nbest.
empty_source:
	${GEN_FEATURE_VALUES} LengthFF 0 <(echo -n) <(seq 1 25) 2>&1 \
		| grep 'Error: Problem with the input sizes (NB: 25, S: 0)' --quiet

# gen_feature_values should report an error if it is given a none empty source
# file with an empty nbest list.
empty_nbest:
	${GEN_FEATURE_VALUES} LengthFF 0 <(seq 1 5) <(echo -n) 2>&1 \
		| grep 'Error: Problem with the input sizes (NB: 0, S: 5)' --quiet

# gen_feature_values should report an error if the nbest list's size is not a
# multiple of the source's size.
invalid_nbest_size:
	${GEN_FEATURE_VALUES} LengthFF 0 <(seq 1 5) <(seq 1 13) 2>&1 \
		| grep "Error: Nbest list's size(13) is not a multiple of source's size(5)." --quiet

