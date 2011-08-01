#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# $Id$

# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


-include Makefile.params

THRESHOLD := 4.5
SIGPRUNE := $(shell which sigprune.sh)

vpath jpt.%.en-fr.gz src
vpath tm-train%.gz src

SHELL = /bin/bash

.PHONY: all
all: testsuite


.PHONY: clean
clean:
	${RM} log.unittest* runpara_cmds run-parallel-logs-local unittest*.gz cnts.jpt.ibm2.tm-train.en-fr.gz
	${RM} -r run-p.*  *.tpsa  parallelize.pl.*  TEST.*



.PHONY: testsuite
testsuite: unittest0
testsuite: unittest1
testsuite: unittest2
testsuite: unittest3
testsuite: unittest4
testsuite: unittest5
testsuite: unittest6
testsuite: unittest7
testsuite: unittest8



# Test out the parallelism when calling phrasepair-contingency within sigpurne.sh.
# Also make sure that run-parallel's arguments are correctly passed down.
UNITTEST0 := -d -nocluster -psub "-2 -N U0_" -p TEST.
.PHONY: unittest0
unittest0: unittest0.gz
	[[ ! -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file mustn't exsit after.
	[[ `grep -c 'DONE (Primary' log.$@` == 4 ]] || ! echo "parallelize.pl should have created 4 jobs." >&2
	[[ `grep 'DONE (Primary' log.$@ | egrep -c 'parallelize.pl\.[0-9]+'` == 4 ]] || ! echo "Check if $@ used parallelize.pl?" >&2
	grep 'Creating the co-occurence counts.' log.$@
	grep -F 'SAVE_ARGS = $(subst ",,${UNITTEST0})' log.$@
	grep 'PSUBOPTS  =  -2 -N U0_' log.$@
	diff -q <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz)

unittest0.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz
	-rm -f cnts.jpt.ibm2.tm-train.en-fr.gz
	export PORTAGE_NOCLUSTER=1; ${SIGPRUNE} -d -rp '${UNITTEST0}' -n 4 -np 2 -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)


# Don't keep co-occurrence count file and doesn't exist before.
# Also make sure that psub's arguments are correctly passed down.
UNITTEST1 := -psub '-N TEST -3'
.PHONY: unittest1
unittest1: unittest1.gz
	[[ ! -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file mustn't exsit after.
	grep 'Creating the co-occurence counts.' log.$@
	grep 'Requested 3 CPUs per worker' log.$@
	diff -q <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz)

unittest1.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest0.gz
	-rm -f cnts.jpt.ibm2.tm-train.en-fr.gz
	${SIGPRUNE} ${UNITTEST1} -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)


# Keep but cnts.jpt doesn't already exist.
.PHONY: unittest2
unittest2: unittest2.gz
	[[ -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  #  Co-occurrence count file must exist after.
	grep 'Creating the co-occurence counts.' log.$@
	diff -q <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz)
	diff -q <(zcat -f cnts.jpt.ibm2.tm-train.en-fr.gz | cut -f2-) <(zcat -f ref/cnts.jpt.ibm2.tm-train.en-fr.gz | cut -f2-)

unittest2.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest1.gz
	${SIGPRUNE} -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)


# Co-occurrence count file already exist and we want to keep it.
.PHONY: unittest3
unittest3: unittest3.gz
	[[ -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must also exist after.
	grep 'Warning: Reusing the co-occurence count file' log.$@
	diff -q <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz)

unittest3.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest2.gz
	[[ -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must exist before.
	${SIGPRUNE} -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)


# Co-occurrence count file already exist and we don't want to keep it.
.PHONY: unittest4
unittest4: unittest4.gz
	[[ -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must also exist after.
	grep 'Warning: Reusing the co-occurence count file' log.$@
	diff -q <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz)

unittest4.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest3.gz
	[[ -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must exist before.
	${SIGPRUNE} -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)


# Keep or filter-out <1,1,1> 
.PHONY: unittest5 unittest6
unittest5 unittest6: unittest%: unittest%.gz
	grep 'Warning: Reusing the co-occurence count file' log.$@
	diff -q <(zcat -f $<) <(zcat -f ref/$<)

unittest5.gz: THRESHOLD=a-e
unittest6.gz: THRESHOLD=a+e
unittest5.gz unittest6.gz: unittest%.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest4.gz
	[[ -e "cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must exist before.
	${SIGPRUNE} -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)


# Threshold values should be positive.
.PHONY: unittest7
unittest7:  unittest7.gz
unittest7.gz:  jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest4.gz
	${SIGPRUNE} -keep -threshold -10 $+ $@ 2>&1 | grep 'fatal error: Your threshold should be a positive value'


# Threshold values should be great than 0.
.PHONY: unittest8
unittest8:  unittest8.gz
unittest8.gz:  jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest4.gz
	${SIGPRUNE} -keep -threshold 0 $+ $@ 2>&1 | grep 'fatal error: Your threshold should be a positive value'

