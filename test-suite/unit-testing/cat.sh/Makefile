#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file testsuite
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


CAT_SH := cat.sh

-include Makefile.params


SHELL := bash

.PHONY: all
all: testsuite

TEMP_FILES=log.* tm-train*lc.gz
TEMP_DIRS=ibm2.tm-train.fr_given_en.gz.tmp*
include ../Makefile.incl


.PHONY: testsuite
testsuite: unittest0


tm-train_en.lc.gz tm-train_fr.lc.gz: %:
	touch $@


.PHONY: unittest0
unittest0: %: log.%
	@grep --quiet 'Starting iteration 14' $+ || echo "Where is the last iteration, number 14?" >&2
	@[[ `grep -c 'Starting iteration' $+` -eq $$((7+7)) ]] || echo "There are some missing iterations, we expect 14 iterations!" >&2
	@[[ `grep -c '\-ibm1 -count-only' $+` -eq $$((7*6)) ]] || echo "There should be 7 iterations of 6 workers for ibm1!" >&2
	@[[ `grep -c '\-ibm2 -count-only' $+` -eq $$((7*8)) ]] || echo "There should be 7 iterations of 8 workers for ibm2!" >&2
	@[[ `egrep -c 'ibm2.tm-train.fr_given_en.gz/iter[0-9]+.merge' $+` -eq $$((7+7)) ]] || echo "There should be 7 + 7 merge steps!" >&2
	@[[ `egrep -c 'run-parallel.sh ibm2.tm-train.fr_given_en.gz/iter[0-9]+.jobs.ibm1 3' $+` -eq 7 ]] || echo "There should be 7 count jobs for ibm1" >&2
	@[[ `egrep -c 'run-parallel.sh ibm2.tm-train.fr_given_en.gz/iter[0-9]+.jobs 4' $+` -eq 7 ]] || echo "There should be 7 count jobs for ibm2" >&2

log.unittest0: tm-train_en.lc.gz tm-train_fr.lc.gz
	${CAT_SH} \
		-v \
		-notreally \
		-n 8 -pn 4 \
		-1n 6 -1pn 3 \
		-n1 7 -n2 7 \
		-bin ibm2.tm-train.fr_given_en.gz \
		$+ 2>&1 \
	| sed 's#.tmp[0-9]\+##g; s#\(Starting iteration [0-9]\+\).*#\1#' \
		&> $@
