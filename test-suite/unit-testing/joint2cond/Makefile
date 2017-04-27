#!/usr/bin/make -f
# vim:noet:ts=3
# @file
# @brief
#
# @author Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada



SHELL = bash

CYGWIN = #cygwin.

JOINT2COND_PHRASE_TABLES ?= joint2cond_phrase_tables
#JOINT2COND_PHRASE_TABLES = ../../../src/tm/joint2cond_phrase_tables

SMOOTHERS ?= -s RFSmoother  -s ZNSmoother  -s "KNSmoother 3"
SMOOTHERS_DESC ?= rf-zn-kn3

#SMOOTHERS = -s JointFreqs  -s Sig2Probs  -s LeaveOneOut
#SMOOTHERS_DESC = jf-s2p-loo
#
#SMOOTHERS = -s GTSmoother  -s IBM1Smoother  -s IBMSmoother
#SMOOTHERS_DESC = gt-ibm1-ibm
#
#SMOOTHERS = -s IndicatorSmoother  -s CollapsingSmoother
#SMOOTHERS_DESC = i-c
#
#SMOOTHERS = -s "MarginalFreqs 1"  -s "MarginalFreqs 2"
#SMOOTHERS_DESC = mf1-mf2

REDUCE_MEMORY = -reduce-mem
#REDUCE_MEMORY = -no-reduce-mem

TMTEXT = #-tmtext

TMDBDUMP = #-tmdbdump

PRUNE = #-prune1 300	# Note: at most 202 translations

JOINT2MULTI_CPT ?= joint2multi_cpt
#JOINT2MULTI_CPT = ../../../src/tm/joint2multi_cpt

M_SMOOTHERS ?= -s RFSmoother  -s ZNSmoother
M_SMOOTHERS_DESC ?= rf-zn

GEN_PHRASE_TABLES ?= gen_phrase_tables
#GEN_PHRASE_TABLES = ../../../src/tm/gen_phrase_tables

.PHONY: all cpt multi gen

all: cpt multi gen prune1_2 smoothers

cpt: cpt.ibm2-${SMOOTHERS_DESC}.train.en2fr.gz

# We leave smoother3 out, because it is not stable across systems
smoothers: smoother1 smoother2 smoother4

multi: cpt.multi.ibm2-${M_SMOOTHERS_DESC}.train.en2fr.gz

gen: out.jpt.ibm2.train.en-fr

prune1_2: cpt.ibm2-rf-prune1_2.train.en2fr.gz

vpath %.al data
vpath ibm2.% data
vpath jpt.% data

TEMP_FILES=cpt.ibm2-${SMOOTHERS_DESC}.train.prune1_fwd* sort*\
           cpt.*ibm2-*.train.en2fr* log.cpt.* log.out.* out.jpt.ibm2.train.en-fr* \
           stats.out classes.out.lang* valgrind. cpt.KN_stats.uu2vv
include ../Makefile.incl

cpt.ibm2-${SMOOTHERS_DESC}.train.en2fr.gz: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	${RM} $@
	${JOINT2COND_PHRASE_TABLES} \
		-ibm 2 \
		-v -i -z \
		-1 en -2 fr \
		${SMOOTHERS} \
		-multipr fwd \
		-o $(subst .en2fr.gz, , $@) \
		-ibm_l2_given_l1  $(word 1, $^) \
		-ibm_l1_given_l2  $(word 2, $^) \
		${TMTEXT} \
		${TMDBDUMP} \
		${REDUCE_MEMORY} \
		${PRUNE} \
		$(word 3, $^) \
		>& log.cpt.ibm2-${SMOOTHERS_DESC}.train.en2fr
	diff-round.pl -p 5 $@ ref/$@ -q

cpt.multi.ibm2-${M_SMOOTHERS_DESC}.train.en2fr.gz: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.1.gz jpt.ibm2.train.en-fr.2.gz
	${RM} $@
	${JOINT2MULTI_CPT} \
		-ibm 2 \
		-v -i -z \
		-1 en -2 fr \
		${M_SMOOTHERS} \
		-o $(subst .en2fr.gz, , $@) \
		-ibm_l2_given_l1  $(word 1, $^) \
		-ibm_l1_given_l2  $(word 2, $^) \
		${PRUNE} \
		$(word 3, $^) $(word 4, $^) \
		>& log.cpt.multi.ibm2-${M_SMOOTHERS_DESC}.train.en2fr
	diff-round.pl -p 5 -sort $@ ref/$@ -q

# NOTE: between g++-4.[234] and g++4.5, out.jpt.ibm2.train.en-fr is different.
# Since the autobuild uses g++-4.2 we need to be lenient about the differences between the ref which was generate with g++-4.5.
# NOTE: out.jpt.ibm2.train.en-fr generated on balzac using g++-4.2 is not equal to out.jpt.ibm2.train.en-fr generated on leclerc using g++-4.2.
out.jpt.ibm2.train.en-fr: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz train_en.al train_fr.al
	${GEN_PHRASE_TABLES} \
		-write-al none \
		-v -z -j -w 1 -m 8 -ibm 2 -1 en -2 fr $+ \
		> $@ 2> log.out.jpt.ibm2.train.en-fr
	@if diff-round.pl -p 5 -sort $@ ref/${CYGWIN}$@ -q ; then \
		echo diff-round.pl -p 5 -sort $@ ref/${CYGWIN}$@ -q: OK; \
	else \
		if diff-round.pl -p 5 -sort $@ ref/old.$@ -q ; then \
			echo diff-round.pl -p 5 -sort $@ ref/old.$@ -q: OK, $@ matches old reference; \
		else \
			echo Error: $@ matches neither the old nor the new reference; \
			false; \
		fi; \
	fi

#	[[ `diff <(LC_ALL=C sort $@) <(zcat ref/${CYGWIN}$@.gz | LC_ALL=C sort) | grep -cE '^[<>]'` -le 350 ]]

# prune1_2 exercices the tie-breaking part of the code
cpt.ibm2-rf-prune1_2.train.en2fr.gz: jpt.ibm2.train.en-fr.gz
	${RM} $@
	${JOINT2COND_PHRASE_TABLES} \
		-no-sort -force -prune1 2 -v -i -z -reduce-mem \
		-s RFSmoother -multipr fwd \
		-o $(subst .en2fr.gz, , $@) \
		$^ \
		>& log.$(subst .gz,,$@)
	diff-round.pl -p 5 $@ ref/$@ -q
	zcat $@ | LC_ALL=C sort -c

valgrind: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	valgrind --tool=massif --depth=6 \
	${JOINT2COND_PHRASE_TABLES} \
		-ibm 2 \
		-v -i -z \
		-1 en -2 fr \
		${SMOOTHERS} \
		-multipr fwd \
		-o cpt.ibm2-${SMOOTHERS_DESC}.train \
		-ibm_l2_given_l1  $(word 1, $^) \
		-ibm_l1_given_l2  $(word 2, $^) \
		${REDUCE_MEMORY} \
		$(word 3, $^)

smoother1: cpt
	${MAKE} cpt SMOOTHERS="-s JointFreqs  -s Sig2Probs  -s LeaveOneOut" SMOOTHERS_DESC=jf-s2p-loo

smoother2: cpt
	${MAKE} cpt SMOOTHERS="-s GTSmoother  -s IBM1Smoother  -s IBMSmoother" SMOOTHERS_DESC=gt-ibm1-ibm

smoother3: cpt
	${MAKE} cpt SMOOTHERS="-s IndicatorSmoother  -s PCSmoother" SMOOTHERS_DESC=i-c

smoother4: cpt
	${MAKE} cpt SMOOTHERS='-s "MarginalFreqs 1"  -s "MarginalFreqs 2"' SMOOTHERS_DESC=mf1-mf2


.PHONY: arg_checking
arg_checking: prune1_both prune1_rev prune1_fwd

# Let's make sure this testsuite checks joint2cond_phrase_tables's arguments.
all: arg_checking

# Prune1 is invalid when generating multiprobs in both direction.
prune1_both: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	${JOINT2COND_PHRASE_TABLES} \
		-prune1 10 \
		-ibm 2 \
		-v -i -z -no-sort \
		-1 en -2 fr \
		${SMOOTHERS} \
		-multipr both \
		-o cpt.ibm2-${SMOOTHERS_DESC}.train.$@ \
		-ibm_l2_given_l1  $(filter %.fr_given_en.gz, $+) \
		-ibm_l1_given_l2  $(filter %.en_given_fr.gz, $+) \
		${REDUCE_MEMORY} \
		$(filter %.en-fr.gz, $+) 2>&1 \
	| egrep "Error: prune1\(w\) is not valid with -multipr rev or -multipr both." --quiet

# Prune1 is invalid when generating multiprobs in rev direction.
prune1_rev: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	${JOINT2COND_PHRASE_TABLES} \
		-prune1 10 \
		-ibm 2 \
		-v -i -z -no-sort \
		-1 en -2 fr \
		${SMOOTHERS} \
		-multipr rev \
		-o cpt.ibm2-${SMOOTHERS_DESC}.train.$@ \
		-ibm_l2_given_l1  $(filter %.fr_given_en.gz, $+) \
		-ibm_l1_given_l2  $(filter %.en_given_fr.gz, $+) \
		${REDUCE_MEMORY} \
		$(filter %.en-fr.gz, $+) 2>&1 \
	| egrep "Error: prune1\(w\) is not valid with -multipr rev or -multipr both." --quiet

# Prune1 is only valid when generating multiprobs in fwd direction.
prune1_fwd: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	${JOINT2COND_PHRASE_TABLES} \
		-force \
		-prune1 10 \
		-ibm 2 \
		-v -i -z \
		-1 en -2 fr \
		${SMOOTHERS} \
		-multipr fwd \
		-o cpt.ibm2-${SMOOTHERS_DESC}.train.$@ \
		-ibm_l2_given_l1  $(filter %.fr_given_en.gz, $+) \
		-ibm_l1_given_l2  $(filter %.en_given_fr.gz, $+) \
		${REDUCE_MEMORY} \
		$(filter %.en-fr.gz, $+) >& /dev/null

# Validate  KNsmoother state
cpt.KN_stats.uu2vv:  %.uu2vv:
	echo -e "A ||| b ||| 4\nA ||| c ||| 5\nA ||| e ||| 9\nB ||| b ||| 6\nD ||| d ||| 1" \
	| ${JOINT2COND_PHRASE_TABLES} \
		-vvi \
		-1 uu \
		-2 vv \
		-o $* \
		-s 'KNSmoother' \
		-write-smoother-state \
		-multipr fwd

all:  cpt.KN_stats.uu2vv.test
.PHONY:  cpt.KN_stats.uu2vv.test
cpt.KN_stats.uu2vv.test:  %.test:  %  ref/cpt.KN_stats.uu2vv
	diff $+ -q
