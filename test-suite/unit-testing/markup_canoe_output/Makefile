# @file Makefile to test markup_canoe_output
# @author Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

.SECONDARY:

export PATH:=~/workspace/PORTAGEshared/src/preprocessing:${PATH}

.PHONY: all
all: whitespace_test

TEMP_FILES=whitespace.P.tags.tok log.whitespace
include ../Makefile.incl

export PORTAGE_INTERNAL_CALL=1

whitespace_test: OPTS=-v -xtags

.PHONY: whitespace_test

whitespace_test: %_test: in/%.Q.tags.tok in/%.P.tok in/%.p.pal
	markup_canoe_output ${OPTS} $+ > $*.P.tags.tok 2> log.$*
	diff $*.P.tags.tok ref/$*.P.tags.tok -q


########################################
# HELPERS

in/whitespace.Q.tags.tok: in/whitespace.Q.tags.txt
	utokenize.pl -noss -xtags $< > $@
