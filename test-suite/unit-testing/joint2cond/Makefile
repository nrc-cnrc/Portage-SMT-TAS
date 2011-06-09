#!/usr/bin/make -f
# vim:noet:ts=3

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

.PHONY: all clean cpt multi gen

all: cpt multi gen prune1_2
	@echo All tests PASSED \(assuming you ran with -B\)

cpt: cpt.ibm2-${SMOOTHERS_DESC}.train.en2fr.gz

multi: cpt.multi.ibm2-${M_SMOOTHERS_DESC}.train.en2fr.gz

gen: out.jpt.ibm2.train.en-fr

prune1_2: cpt.ibm2-rf-prune1_2.train.en2fr.gz

vpath %.al data
vpath ibm2.% data
vpath jpt.% data

clean:
	${RM} cpt.ibm2-${SMOOTHERS_DESC}.train.prune1_fwd*
	$(RM) cpt.*ibm2-*.train.en2fr* log.cpt.* log.out.* out.jpt.ibm2.train.en-fr* \
		stats.out classes.out.lang* valgrind.

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
	diff-round.pl $@ ref/$@ -q

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
	diff-round.pl -sort $@ ref/$@ -q

out.jpt.ibm2.train.en-fr: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz train_en.al train_fr.al
	${GEN_PHRASE_TABLES} \
		-v -z -j -w 1 -m 8 -ibm 2 -1 en -2 fr $+ \
		> $@ 2> log.out.jpt.ibm2.train.en-fr
	diff-round.pl -sort $@ ref/${CYGWIN}$@.gz -q

# prune1_2 exercices the tie-breaking part of the code
cpt.ibm2-rf-prune1_2.train.en2fr.gz: jpt.ibm2.train.en-fr.gz
	${RM} $@
	${JOINT2COND_PHRASE_TABLES} \
		-no-sort -force -prune1 2 -v -i -z -reduce-mem \
		-s RFSmoother -multipr fwd \
		-o $(subst .en2fr.gz, , $@) \
		$^ \
		>& log.$(subst .gz,,$@)
	diff-round.pl $@ ref/$@ -q
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



.PHONY: arg_checking
arg_checking: prune1_both prune1_rev prune1_fwd

# Let's make sure this testsuite checks joint2cond_phrase_tables's arguments.
all: arg_checking

# Prune1 is invalid when generating multiprobs in both direction.
prune1_both: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	[[ `${JOINT2COND_PHRASE_TABLES} \
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
		$(filter %.en-fr.gz, $+) 2>&1` =~ "Error: prune1\(w\) is not valid with -multipr rev or -multipr both." ]]

# Prune1 is invalid when generating multiprobs in rev direction.
prune1_rev: ibm2.train.fr_given_en.gz ibm2.train.en_given_fr.gz jpt.ibm2.train.en-fr.gz
	[[ `${JOINT2COND_PHRASE_TABLES} \
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
		$(filter %.en-fr.gz, $+) 2>&1` =~ "Error: prune1\(w\) is not valid with -multipr rev or -multipr both." ]]

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

