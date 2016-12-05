#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief Validates that canoe can produce a file containing triangular arrays
#        for every source sentence.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

CANOE := canoe

-include Makefile.params

TEMP_FILES := *.out

include ../Makefile.incl

vpath canoe.ini.cow  ${PORTAGE}/test-suite/systems/toy-regress-en2fr
vpath input  src
vpath triangularArray.ref ref

.DEFAULT_GOAL := all
.PHONY: all
all:  noTriangularArray
all: triangularArray.check


noTriangularArray:  canoe.ini.cow  input
	-rm *.out
	${CANOE} -f $(filter %canoe.ini.cow,$+) < $(filter %input,$+)
	[[ ! -s triangularArray.out ]] || ! echo "Error: There should be a triangular array file." >&2

triangularArray.out:  noTriangularArray
triangularArray.out:  canoe.ini.cow  input
	${CANOE} -f $(filter %canoe.ini.cow,$+) -triangularArrayFilename $@  < $(filter %input,$+)
	[[ -s triangularArray.out ]] || ! echo "Error: Missing the triangular array output." >&2

triangularArray.check:  triangularArray.out  triangularArray.ref
	diff-round.pl -q -sort $+
