#!/usr/bin/make -f
# @file Makefile
# @brief
#
# @author Eric Joanis and Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

# We don't want subjobs submitted here, since they're too small.
export PORTAGE_NOCLUSTER=1

.SECONDARY:

.PHONY: all
all:  test.locking test.empty test.too_small test.s1t1 test.custom_cmds \
      test.bad_init test.2_init


TEMP_DIRS = context.* workdir.test.*
TEMP_FILES = s1.rule dev1_* log.*
include ../Makefile.incl


# Absolute pathname needed for $TOY_SYSTEM
TOY_SYSTEM ?= $(abspath ${PORTAGE}/test-suite/systems/toy-regress-en2fr)

TUTORIAL_DATA ?= ${PORTAGE}/test-suite/tutorial-data

TIME_TAG = test
define TIMEFORMAT

${TIME_TAG}:  real %3lR	user %3lU	sys %3lS
endef
export TIMEFORMAT

# function TIME_FORMAT <tag>
TIME_FORMAT = TIMEFORMAT=$$'\n'"${1}:  "$$'real %3lR\tuser %3lU\tsys %3lS'


# Set up contexts for testing incr-update.sh

context.main context.custom_cmds: context.%:
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/* .
	rm $@/incremental.config
	mv $@/canoe.ini.cow $@/canoe.ini.cow.orig
	cd $@ && configtool -p args:"-stack 2000" canoe.ini.cow.orig > canoe.ini.cow
	rm $@/canoe.ini.cow.orig
	cp config.$* $@/incremental.config

context.bad:			# missing incremental.config
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/* .
	rm $@/incremental.config

# Test creation of an incremental model with an empty corpus
.PHONY: test.empty check.test.empty

test.empty: %: check.%

check.test.empty: check.%: workdir.%
	grep -q __DUMMY__ $</lm.incremental_fr
	grep -q "[lock]" $</canoe.ini.cow
	grep -q cpt-incr.mixtm $</canoe.ini.cow
	grep -q lm-incr.mixlm $</canoe.ini.cow

workdir.test.empty: TIME_TAG = incr-update empty

workdir.test.empty: context.main
	mkdir -p $@
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v /dev/null >& log.update-empty

# Test creation of an incremental model with too small a target corpus for
# regular LM creation
.PHONY: test.too_small check.test.too_small

test.too_small: %: check.%

check.test.too_small: check.%: workdir.%
	grep -q __DUMMY__ $</lm.incremental_fr

workdir.test.too_small: TIME_TAG = incr-update too_small

workdir.test.too_small: context.main
	mkdir -p $@
	echo -e "`date +"%F %T"`\t.\t." > $@/corpora
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v corpora >& log.update-too_small

# Create a model with the s1 t1 sentence pair update, and redecode to see
# "Mr. Boazek" correctly translated to "M. Baozeck".
.PHONY: test.s1t1 check.test.s1t1

test.s1t1: %: check.%

check.test.s1t1: check.%: workdir.%/s1.out.incr
	grep -i Baozeck $<

workdir.test.s1t1: TIME_TAG = incr-update s1t1

workdir.test.s1t1: context.main
	mkdir -p $@
	paste <(echo `date +"%F %T"`) s1 t1 > $@/corpora
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v corpora >& log.update-s1t1

s1.rule: %.rule: %
	utokenize.pl -noss -lang en < $< | utf8_casemap -c l | canoe-escapes.pl > $@

workdir.test.s1t1/s1.out.incr: TIME_TAG = canoe s1t1

workdir.test.s1t1/s1.out.incr: workdir.test.s1t1 s1.rule
	time canoe -f $</canoe.ini.cow -input s1.rule > $@ 2> $</log.s1.out.incr

# Use custom commands in the config and make sure they work
.PHONY: test.custom_cmds check.test.custom_cmds

test.custom_cmds: %: check.%

check.test.custom_cmds: check.%: workdir.%
	grep -q Boazek $</cpt.incremental.en2fr
	grep -q KCEZOAB $</cpt.incremental.en2fr
	grep -q "ERTSINIM UA ELLEVUON" $</lm.incremental_fr

workdir.test.custom_cmds: TIME_TAG = incr-update custom cmds

workdir.test.custom_cmds: context.custom_cmds
	mkdir -p $@
	paste <(echo `date +"%F %T"`) s1 t1 > $@/corpora
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v corpora >& log.update-custom_cmds

# Test initialization failure
.PHONY: test.bad_init check.test.bad_init

test.bad_init: %: check.%

check.test.bad_init: check.%: workdir.%
	! ls $</canoe.ini.cow

workdir.test.bad_init: TIME_TAG = incr-update bad_init

workdir.test.bad_init: context.bad
	mkdir -p $@
	cd $@ && ! time incr-update.sh -c ../$</canoe.ini.cow -v /dev/null >& log.update-bad_init \
		&& echo "Failure for a bad context occurred as expected." >&2 \
		|| ! echo "ERROR: Failure was expected for a bad context, but it did not occur." >&2

# Test double initialization
.PHONY: test.2_init check.test.2_init

test.2_init: %: check.%

check.test.2_init: check.%: workdir.%
	grep -q __DUMMY__ $</lm.incremental_fr

workdir.test.2_init: TIME_TAG = incr-update 2_init

workdir.test.2_init: context.main
	mkdir -p $@
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v /dev/null >& log.update-2_init
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v /dev/null >> log.update-2_init 2>&1

# Test locking
.PHONY: test.locking check.test.locking

test.locking: %: check.%

check.test.locking: check.%: workdir.%
	bleumain $</dev1.out.0 dev1_fr.tok > $</log.dev1.out.0.bleu
	bleumain $</dev1.out.incr dev1_fr.tok > $</log.dev1.out.incr.bleu
	grep Human $</*.bleu

# Prepare the dev1 files for passing to the decoder.
dev1_en.rule: %_en.rule: $(addprefix ${TUTORIAL_DATA}/%, _en.raw _fr.raw)
	utokenize.pl -noss -lang en < $< | utf8_casemap -c l | canoe-escapes.pl > $@
	head -30 $(filter %$*_en.raw,$+) > $*_en.raw
	head -30 $(filter %$*_fr.raw,$+) > $*_fr.raw
	utokenize.pl -noss -lang en < $*_en.raw | utf8_casemap -c l > $*_en.tok
	utokenize.pl -noss -lang fr < $*_fr.raw | utf8_casemap -c l > $*_fr.tok
	canoe-escapes.pl < $*_en.tok > $@

workdir.test.locking: TIME_TAG = locking

workdir.test.locking: context.main dev1_en.rule
	mkdir -p $@
	cd $@ && time incr-update.sh -c ../$</canoe.ini.cow -v /dev/null >& log.locking-init
	cd $@ && ../run-locking-test.sh >& ../log.locking-test
