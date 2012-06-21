#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# $Id$

# @file Makefile
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


TUNE := tune.py
# The optimizers are roughly ordered from most expensive to cheapest, so we
# start by the expensive ones and don't have to wait as long for the results.
OPTIMIZERS := lmira powell mira pro expsb svm
#export PORTAGE_NOCLUSTER=1

-include Makefile.params

vpath %.rule  src/
vpath %.lc    src/

#.NOTPARALLEL:
.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


.PHONY: clean
clean:
	${RM} canoe.tune* decode-config* summary* rescore-weights
	${RM} -r foos* logs log.canoe.* canoe-parallel.*
	${RM} -r tunedir.*



.PHONY: testsuite
testsuite:  ${OPTIMIZERS}  powell.postProcessed
	@echo All tests ran OK '(but output is not checked in this test suite)'.


.PHONY: ${OPTIMIZERS}
${OPTIMIZERS}:  %:  canoe.tune.%


canoe.tune.%:  dev1_en.rule  dev1_fr.lc
	mkdir -p tunedir.$*
	cd tunedir.$*; \
	   run-parallel.sh -psub -4 -c \
	   ${TUNE} --debug -v --workdir=foos -o ../$@ -f ../src/canoe.ini ${EXTRA} -p 4 -c 1 -m 4 -a $* $(foreach f,$+,../$f) >& ../log.$@
	mv tunedir.$*/summary summary.$*
	mv tunedir.$*/summary.wts summary.$*.wts

# Validate that the post processing was applied to the nbest list.  We will uppercase the letter a.
canoe.tune.powell:  EXTRA=--post \'sed "s/a/A/g"\'
powell.postProcessed:  %.postProcessed:  canoe.tune.%
	@[[ "`zgrep -c a tunedir.$*/foos/allnbests.gz`" -eq 0 ]] \
	|| ! echo "ERROR: Post processing didn't happened." >&2

