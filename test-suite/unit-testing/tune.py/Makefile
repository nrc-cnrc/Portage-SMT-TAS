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
OPTIMIZERS := powell mira pro expsb svm lmira
export PORTAGE_NOCLUSTER=1

-include Makefile.params

vpath %.rule  src/
vpath %.lc    src/

.NOTPARALLEL:
.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


.PHONY: clean
clean:
	${RM} canoe.tune* decode-config* summary* rescore-weights
	${RM} -r foos* logs log.canoe.* canoe-parallel.*



.PHONY: testsuite
testsuite:  ${OPTIMIZERS}
	@echo All tests ran OK '(but output is not checked in this test suite)'.


.PHONY: ${OPTIMIZERS}
${OPTIMIZERS}:  %:  canoe.tune.%


canoe.tune.%:  dev1_en.rule  dev1_fr.lc
	${TUNE} --debug -v --workdir=foos.$* -o $@ -f src/canoe.ini -p 3 -c 1 -m 5 -a $* $+ >& log.$@
	mv summary summary.$*
	mv summary.wts summary.$*.wts

