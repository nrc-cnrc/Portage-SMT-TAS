# vim:noet:list
# Makefile - Test the various implementations and options we have for LMs
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

LMPREFIX ?= test
LMEXT ?= lm
ARPALMEXT ?= lm
INPUT ?= test.in
LM = ${LMPREFIX}.${LMEXT}

# runs the test suite over a variety of input files and lms.
all: case1 case2 case3 case4 case5 case6 case7 unk

# case2 uses the same LMs as case1, prevent parallelism via a dependence.
case1: case2
	${MAKE} LMPREFIX=test INPUT=test.in testsuite

case2:
	${MAKE} LMPREFIX=test INPUT=test.in2 testsuite

case3:
	${MAKE} LMPREFIX=test2 INPUT=test.in testsuite

# case4 uses the same LMs as case3, prevent parallelism via a dependence.
case4: case3
	${MAKE} LMPREFIX=test2 INPUT=test.in2 testsuite

case5:
	${MAKE} LMPREFIX=test3 INPUT=test.in testsuite

europarl.en.lm:
	-ln -s ../../regress-small-voc/europarl.en.srilm europarl.en.lm

case6: europarl.en.lm
	${MAKE} LMPREFIX=europarl.en INPUT=test.in testsuite

europarl.en.in:
	-ln -s ../../regress-small-voc/lc/europarl.fr-en.en.lowercase europarl.en.in

# case7 uses the same LMs as case6, prevent parallelism via a dependence.
case7: case6 europarl.en.lm europarl.en.in
	${MAKE} LMPREFIX=europarl.en INPUT=europarl.en.in testsuite

# Runs all tests for a given input file and lm file.
testsuite:
	@if [[ "${INPUT}" != "`basename ${INPUT}`" ]]; then \
	   echo Error: INPUT=${INPUT} not in current directory; false; fi
	@if [[ "${LMPREFIX}" != "`basename ${LMPREFIX}`" ]]; then \
	   echo Error: LMPREFIX=${LMPREFIX} not in current directory; false; fi
	@if [[ ! -r ${INPUT} ]]; then echo Error: Cannot read ${INPUT};false;fi
	@if [[ ! -r ${LMPREFIX}.${ARPALMEXT} ]]; then \
	   echo Error: Cannot read ${LMPREFIX}.${ARPALMEXT}; false; fi
	${MAKE} LMEXT=lm ${INPUT}.${LMPREFIX}.lm_cmp
	${MAKE} LMEXT=binlm ${INPUT}.${LMPREFIX}.lm_cmp
	${MAKE} LMEXT=tplm ${INPUT}.${LMPREFIX}.lm_cmp

.SECONDARY:

%.lm_cmp: ${INPUT} ${LM} ref/%.${ARPALMEXT}.ref %.${LMEXT}.out %.${LMEXT}.out-limit %.${LMEXT}.out-per-sent-limit
	@if [[ "$*" != "${INPUT}.${LMPREFIX}" ]]; then \
	   echo Assert failure: "$*" '!=' "${INPUT}.${LMPREFIX}"; false; fi
	gzip -cdqf ref/$*.${ARPALMEXT}.ref | diff - $*.${LMEXT}.out -q
	gzip -cdqf ref/$*.${ARPALMEXT}.ref | diff - $*.${LMEXT}.out-limit -q
	gzip -cdqf ref/$*.${ARPALMEXT}.ref | diff - $*.${LMEXT}.out-per-sent-limit -q
	@echo Tests for $@ with LMEXT=${LMEXT} PASSED

${INPUT}.${LMPREFIX}.${LMEXT}.out: ${INPUT} ${LM}
	time lm_eval ${LM} ${INPUT} > $@ 2> log.$@

${INPUT}.${LMPREFIX}.${LMEXT}.out-limit: ${INPUT} ${LM}
	time lm_eval -limit ${LM} ${INPUT} > $@ 2> log.$@

${INPUT}.${LMPREFIX}.${LMEXT}.out-per-sent-limit: ${INPUT} ${LM}
	time lm_eval -limit -per-sent-limit ${LM} ${INPUT} > $@ 2> log.$@

%.tplm: %.${ARPALMEXT}
	arpalm2tplm.sh -n 4 $< $@ >& log.$@

%.binlm: %.${ARPALMEXT}
	arpalm2binlm $< $@ >& log.$@

TEMP_FILES=*.out *.out-limit *.out-per-sent-limit log.* *.binlm europarl.en.lm europarl.en.in *.lm.unk
TEMP_DIRS=europarl.en.tplm test.tplm test2.tplm test3.tplm europarl.en.tplm.tmp.* test.tplm.tmp.* test2.tplm.tmp.* test3.tplm.tmp.*
include ../Makefile.incl

################################################
# Test lm-add-heuristic-unk.pl

unk: test3.lm.unk_diff test4.lm.unk_diff test5.lm.unk_diff test6.lm.unk_diff unk-errors

%.lm.unk: %.lm
	lm-add-heuristic-unk.pl $< > $@

%_diff: %
	diff $* ref/$*

unk-errors:
	! lm-add-heuristic-unk.pl Makefile
	! lm-add-heuristic-unk.pl -delta alpha test3.lm >& log.unk-bad-delta
