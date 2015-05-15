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
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


SIGPRUNE := sigprune.sh
THRESHOLD := 4.5

-include Makefile.params


vpath jpt.%.en-fr.gz src
vpath tm-train%.gz src

SHELL = /bin/bash

.PHONY: all
all: testsuite


TEMP_FILES=log.* runpara_cmds run-parallel-logs-local unittest*.gz \
           sig.cnts.jpt.ibm2.tm-train.en-fr.gz sig.cnts.jpt.vtab.gz \
           jpt.unittest_j_option_?.gz
TEMP_DIRS=run-p.*  *.tpsa *.tpsa.tmp.* parallelize.pl.* TEST.* sigprune.sh.*
include ../Makefile.incl


.PHONY: testsuite
#testsuite: unittest0
testsuite: unittest1
testsuite: unittest2
testsuite: unittest3
testsuite: unittest4
testsuite: unittest5
testsuite: unittest6
testsuite: unittest7
testsuite: unittest8
testsuite: test_find_vtab test_sig_vtab
testsuite: unittest_j



# Not the reference sigprune jpt but the counts required to create the sigpruned jpt.
reference.counts: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz
	build-tp-suffix-array.sh $(filter %tm-train_en.lc.gz, $+)
	mv tm-train_en.lc.tpsa tm-train.en.lc.tpsa
	build-tp-suffix-array.sh $(filter %tm-train_fr.lc.gz, $+)
	mv tm-train_fr.lc.tpsa tm-train.fr.lc.tpsa
	zcat $(filter %jpt.ibm2.tm-train.en-fr.gz, $+) \
	| phrasepair-contingency --sigfet tm-train en.lc.tpsa fr.lc.tpsa \
	> $@



# Test out the parallelism when calling phrasepair-contingency within sigpurne.sh.
# Also make sure that run-parallel's arguments are correctly passed down.
UNITTEST0_OPTS := -rp "-d -nocluster -p TEST." -psub "-2 -N U0_"
.PHONY: unittest0
unittest0: unittest0.gz
	[[ `grep 'DONE (Primary' log.$@ | grep -c 'build-tp-suffix-array.sh'` == 4 ]] || ! echo "sigprune.sh should have created 2 mmsufa.tpsa" >&2
	[[ `grep 'DONE (Primary' log.$@ | grep -c 'phrasepair-contingency --sigfet'` == 8 ]] || ! echo "Unexpected number of contingency jobs." >&2
	grep --quiet 'Creating memory mapped suffix arrays.' log.$@
	grep --quiet 'Calculating phrase pairs co-occurences for each shard.' log.$@
	grep --quiet 'Calculating phrase pairs co-occurences for the entire joint phrase table.' log.$@
	grep -F 'SAVE_ARGS = -d -nocluster -p TEST. -psub  -2 -N U0_' log.$@
	grep 'PSUBOPTS  =  -2 -N U0_' log.$@
	diff <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz) -q

# use -w 1000 -W 1200 to force creating two sets of mmsufa, 4 in total
# NOTE: we will test if the intermediate count file exist here in order to be able to rerun make without error.
unittest0.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz
	-rm -f sig.cnts.jpt.ibm2.tm-train.en-fr.gz
	export PORTAGE_NOCLUSTER=1; ${SIGPRUNE} -v -w 1000 -W 1200 -d ${UNITTEST0_OPTS} -n 4 -np 2 -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)
	[[ ! -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must not exist after.



# Don't keep co-occurrence count file and doesn't exist before.
# Also make sure that psub's arguments are correctly passed down.
UNITTEST1 := -psub "-N TEST -3"
.PHONY: unittest1
unittest1: unittest1.gz
	grep --quiet 'Creating memory mapped suffix arrays.' log.$@
	grep --quiet 'Calculating phrase pairs co-occurences for each shard.' log.$@
	grep --quiet 'Calculating phrase pairs co-occurences for the entire joint phrase table.' log.$@
	[[ `hostname` != "balzac" ]] || grep 'Requested 3 CPUs per worker' log.$@
	diff <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz) -q

unittest1.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest0.gz
	-rm -f sig.cnts.jpt.ibm2.tm-train.en-fr.gz
	${SIGPRUNE} ${UNITTEST1} -v -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)
	[[ ! -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must not exist after.



# Keep but sig.cnts.jpt doesn't already exist.
.PHONY: unittest2
unittest2: unittest2.gz
	grep --quiet 'Creating memory mapped suffix arrays.' log.$@
	grep --quiet 'Calculating phrase pairs co-occurences for each shard.' log.$@
	grep --quiet 'Calculating phrase pairs co-occurences for the entire joint phrase table.' log.$@
	diff <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz) -q
	diff <(zcat -f sig.cnts.jpt.ibm2.tm-train.en-fr.gz | cut -f2-) <(zcat -f ref/cnts.jpt.ibm2.tm-train.en-fr.gz | cut -f2-) -q

unittest2.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest1.gz
	-rm -f sig.cnts.jpt.ibm2.tm-train.en-fr.gz
	${SIGPRUNE} -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)
	[[ -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  #  Co-occurrence count file must exist after.



# Co-occurrence count file already exist and we want to keep it.
.PHONY: unittest3
unittest3: unittest3.gz
	grep 'WARNING: Reusing the co-occurence count file' log.$@
	diff <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz) -q

unittest3.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest2.gz
	[[ -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must exist before.
	${SIGPRUNE} -v -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)
	[[ -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must also exist after.



# Co-occurrence count file already exist and we don't want to keep it.
.PHONY: unittest4
unittest4: unittest4.gz
	grep 'WARNING: Reusing the co-occurence count file' log.$@
	diff <(zcat -f $<) <(zcat -f ref/sig.jpt.ibm2.tm-train.en-fr.gz) -q

unittest4.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest3.gz
	[[ -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must exist before.
	${SIGPRUNE} -v -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)
	[[ -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must also exist after.



# Keep or filter-out <1,1,1>
.PHONY: unittest5 unittest6
unittest5 unittest6: unittest%: unittest%.gz
	grep 'WARNING: Reusing the co-occurence count file' log.$@
	diff <(zcat -f $<) <(zcat -f ref/$<) -q

unittest5.gz: THRESHOLD=a-e
unittest6.gz: THRESHOLD=a+e
unittest5.gz unittest6.gz: unittest%.gz: jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest4.gz
	[[ -e "sig.cnts.jpt.ibm2.tm-train.en-fr.gz" ]]  # Co-occurrence count file must exist before.
	${SIGPRUNE} -keep -threshold ${THRESHOLD} $+ $@ 2> log.$(basename $@)



# Threshold values should be positive.
.PHONY: unittest7
unittest7:  unittest7.gz
unittest7.gz:  jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest4.gz
	${SIGPRUNE} -keep -threshold -10 $+ $@ 2>&1 | grep 'fatal error: Your threshold should be a positive value'
	@echo "OK: fatal error as expected"



# Threshold values should be great than 0.
.PHONY: unittest8
unittest8:  unittest8.gz
unittest8.gz:  jpt.ibm2.tm-train.en-fr.gz tm-train_en.lc.gz tm-train_fr.lc.gz | unittest4.gz
	${SIGPRUNE} -keep -threshold 0 $+ $@ 2>&1 | grep 'fatal error: Your threshold should be a positive value'
	@echo "OK: fatal error as expected"



#### vtab testing - make sure token separator is simply [spaces|tab]+, nothing else

test_sig_vtab: sig.cnts.jpt.vtab.gz
	diff <(zcat $<) <(zcat ref/$<)

vpath jpt.vtab  src/
vpath vtab.%    src/
sig.cnts.jpt.vtab.gz: jpt.vtab vtab.en vtab.fr
	${RM} $@
	${SIGPRUNE} -n 1 -np 1 -keep -threshold .1 $+ >& log.$@

test_find_vtab: vtab.en
	build-tp-suffix-array.sh $<
	mmtsa.find --tpsa vtab.en.tpsa v | diff - /dev/null
	mmtsa.find --tpsa vtab.en.tpsa tab | diff - /dev/null
	mmtsa.find --tpsa vtab.en.tpsa test | diff - <(echo 0 1)
	mmtsa.find --tpsa vtab.en.tpsa $$'v\vtab' | diff - <(echo 0 4)
	mmtsa.find --tpsa vtab.en.tpsa $$'a v\vtab' | diff - <(echo 0 3)
	mmtsa.find --tpsa vtab.en.tpsa $$'\t a \t v\vtab ' | diff - <(echo 0 3)



################################################################################
# Testing the -j option.
.PHONY:  unittest_j
unittest_j:  unittest_j_option_1
unittest_j:  unittest_j_option_2
unittest_j:  unittest_j_option_3

jpt.unittest_j_option_%.gz:  jpt.ibm2.tm-train.en-fr.gz
	ln -s $< $@

# Test minimum number of jpt entries per chunks
# When n is not defined sigprune.sh optimized it.  Here, we are expecting 3 chunks of jpt
unittest_j_option_1:  unittest_j_option_1.gz
	@egrep --quiet "Sharding the joint phrase table in 3 shards of ~856 phrase pairs." log.$@

unittest_j_option_1.gz:  %:  jpt.% tm-train_en.lc.gz tm-train_fr.lc.gz
	-rm -f sig.cnts.$<   # We must recreate the sig.cnts file in order to see how many chunks are created.
	export PORTAGE_NOCLUSTER=1; ${SIGPRUNE} -v -w 1000 -W 1200 -np 3 -j 200 -threshold ${THRESHOLD} $+ $@ 2> log.${@:.gz=}
	[[ ! -e "sig.cnts.$<" ]]  # Co-occurrence count file must not exist after.


# Here, we are expecting 1 chunks of jpt of 2569 since it must be a minimum of 2000.
unittest_j_option_2:  unittest_j_option_2.gz
	@egrep --quiet "Sharding the joint phrase table in 1 shards of ~2569 phrase pairs." log.$@

unittest_j_option_2.gz:  %:  jpt.% tm-train_en.lc.gz tm-train_fr.lc.gz
	-rm -f sig.cnts.$<   # We must recreate the sig.cnts file in order to see how many chunks are created.
	export PORTAGE_NOCLUSTER=1; ${SIGPRUNE} -v -w 1000 -W 1200 -np 3 -j 2000 -threshold ${THRESHOLD} $+ $@ 2> log.${@:.gz=}
	[[ ! -e "sig.cnts.$<" ]]  # Co-occurrence count file must not exist after.


# Here, we are expecting 5 chunks of jpt and each jpt chunks should be bigger than the requested minimum size of 500.
unittest_j_option_3:  unittest_j_option_3.gz
	@egrep --quiet "Sharding the joint phrase table in 5 shards of ~513 phrase pairs." log.$@

unittest_j_option_3.gz:  %:  jpt.% tm-train_en.lc.gz tm-train_fr.lc.gz
	-rm -f sig.cnts.$<   # We must recreate the sig.cnts file in order to see how many chunks are created.
	export PORTAGE_NOCLUSTER=1; ${SIGPRUNE} -v -w 1000 -W 1200 -np 30 -j 500 -threshold ${THRESHOLD} $+ $@ 2> log.${@:.gz=}
	[[ ! -e "sig.cnts.$<" ]]  # Co-occurrence count file must not exist after.

