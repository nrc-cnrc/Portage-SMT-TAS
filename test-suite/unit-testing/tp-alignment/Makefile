# @file Makefile
# @brief Testsuite for tp_alignment_build and tp_alignment_dump
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


TEMP_FILES=*.tpa lines
include ../Makefile.incl

all: runtest

runtest:
	${MAKE} test BASE=small
	${MAKE} test BASE=emptylines

test: dump dump1 dump2.tpa dump2.text

BASE?=small

${BASE}.tpa: ${BASE}
	tp_alignment_build ${BASE} ${BASE}.tpa

dump: ${BASE}.tpa ${BASE}
	tp_alignment_dump ${BASE}.tpa | diff ${BASE} -

SHELL=/bin/bash

dump1: ${BASE}.tpa ${BASE}
	N=`wc -l < ${BASE}`; \
	for i in `seq 1 $$N`; do \
	   diff <(tp_alignment_dump ${BASE}.tpa $$((i-1))) <(select-line $$i ${BASE}); \
	   diff <(tp_alignment_dump ${BASE} $$((i-1))) <(select-line $$i ${BASE}); \
	done

lines:
	echo 3 4 5 6 8 9 10 12 | tr ' ' '\n' > lines

dump2.tpa: ${BASE}.tpa ${BASE} lines
	diff <(tp_alignment_dump ${BASE}.tpa 2-5 7-9 11) <(lines.py lines ${BASE})

dump2.text: ${BASE} lines
	diff <(tp_alignment_dump ${BASE} 2-5 7-9 11) <(lines.py lines ${BASE})
