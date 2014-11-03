#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file
# @brief
# 
# @author Samuel Larkin
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada
#

FIXED_TERM2TM := fixed_term2tm.pl

-include Makefile.params

include ../Makefile.incl


.DEFAULT_GOAL := all
.PHONY: all
all:


.PHONY: clean
clean:


.PHONY: no_input_file
all:  no_input_file
no_input_file:
	${FIXED_TERM2TM} -source_column=1 -source=en -target=fr 2>&1 \
	| grep 'You must provide an input file'


.PHONY:  unbalanced
all:  unbalanced
unbalanced:  src/fiches_termino.unbalanced
	${FIXED_TERM2TM} -source=en -target=fr $<


.PHONY: wrong_source_column_index
all:  wrong_source_column_index
wrong_source_column_index:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=3 -source=en -target=fr $< 2>&1 \
	| grep 'Invalid source column index (3)'


.PHONY: invalid_source_column_index
all:  invalid_source_column_index
invalid_source_column_index:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=0 -source=en -target=fr $< 2>&1 \
	| grep 'Invalid source column index (0)'


.PHONY: no_source_language
all:  no_source_language
no_source_language:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -target=fr $< 2>&1 \
	| grep 'You must provide a source language.'


.PHONY: invalid_source_language
all:  invalid_source_language
invalid_source_language:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -source=zz -target=fr $< 2>&1 \
	| grep 'Unsupported source language (zz)'


.PHONY: mismatch_source_language
all:  mismatch_source_language
mismatch_source_language:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -source=fr -target=fr $< 2>&1 \
	| grep 'Language mismatch for source language(file: eng, cli: fr).'


.PHONY: no_target_language
all:  no_target_language
no_target_language:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -source=en $< 2>&1 \
	| grep 'You must provide a target language.'


.PHONY: invalid_target_language
all:  invalid_target_language
invalid_target_language:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -source=en -target=yy $< 2>&1 \
	| grep 'Unsupported target language (yy)'


.PHONY: mismatch_target_language
all:  mismatch_target_language
mismatch_target_language:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -source=en -target=en $< 2>&1 \
	| grep 'Language mismatch for target language(file: fra, cli: en).'


.PHONY:  test
all:  test
test:  src/fiches_termino.utf8.txt
	${FIXED_TERM2TM} -source_column=1 -source=en -target=fr $< \
	| sort --unique \
	| diff - ref/fiches_termino.utf8.tm --brief
