#!/usr/bin/make -f
# @file Makefile
# @brief
#
# @author Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

INCR_INIT_MODEL := incr-init-model.py

# We don't want subjobs submitted here, since they're too small.
export PORTAGE_NOCLUSTER=1

.NOTPARALLEL:
.SECONDARY:

GOOD_CONTEXTS = $(addprefix context.,1 2 3 4)
BAD_CONTEXTS = $(addprefix context.bad.,1 2)
ALL_CONTEXTS = ${GOOD_CONTEXTS} ${BAD_CONTEXTS}

.PHONY: all
all:  $(addprefix test.,${ALL_CONTEXTS}) test.existing_model


TEMP_DIRS = ${ALL_CONTEXTS} workdir.test.*
TEMP_FILES =
include ../Makefile.incl

# Absolute pathname needed for $TOY_SYSTEM
TOY_SYSTEM ?= $(abspath ${PORTAGE}/test-suite/systems/toy-regress-en2fr)


# Set up contexts for testing incr-init-model.py

# Context 1: single LM, single TM
context.1:
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/* .
	rm $@/incremental.config
	cp src/incremental.config $@/

# Context 2: 2 LMs (main LM first), 2 TMs
# Context 3: 2 LMs (main LM second), 2 TMs
context.2 context.3:
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/{prime.sh,soap-translate.sh,ce_model.cem,md5,README} .
	mkdir -p $@/models
	cd $@/models && ln -sf ${TOY_SYSTEM}/models/{ldm,tc} .
	mkdir -p $@/models/lm
	cd $@/models/lm && ln -sf ${TOY_SYSTEM}/models/lm/* .
	cd $@/models/lm && ln -sfT ${TOY_SYSTEM}/models/lm/lm-train_fr*.tplm test_fr.tplm
	mkdir -p $@/models/tm
	cd $@/models/tm && ln -sf ${TOY_SYSTEM}/models/tm/* .
	cd $@/models/tm && ln -sfT ${TOY_SYSTEM}/models/tm/cpt.*.en2fr.tppt cpt.test.en2fr.tppt
	cp src/incremental.config $@/
	cp src/$@.canoe.ini.cow $@/canoe.ini.cow

# Context 4: MixLM with 2 component LMs, single TM
context.4:
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/{prime.sh,soap-translate.sh,ce_model.cem,md5,README} .
	mkdir -p $@/models
	cd $@/models && ln -sf ${TOY_SYSTEM}/models/{ldm,tc,tm} .
	mkdir -p $@/models/lm
	cd $@/models/lm && ln -sf ${TOY_SYSTEM}/models/lm/* .
	cd $@/models/lm && ln -sfT ${TOY_SYSTEM}/models/lm/lm-train_fr*.tplm test_fr.tplm
	cp src/$@.test.mixlm $@/models/lm/test.mixlm
	cp src/incremental.config $@/
	cp src/$@.canoe.ini.cow $@/canoe.ini.cow

# Bad Context 1: missing incremental.config
context.bad.1:
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/* .
	rm $@/incremental.config

# Bad Context 2: missing SRC_LANG, TGT_LANG in incremental.config
context.bad.2:
	mkdir -p $@
	cd $@ && ln -sf ${TOY_SYSTEM}/* .
	rm $@/incremental.config
	cp src/incremental.config.bad.nolang $@/incremental.config

# Test each context
.PHONY: $(addprefix test.,${ALL_CONTEXTS})

$(addprefix test.,${ALL_CONTEXTS}): %: check.%

.PHONY: $(addprefix check.test.,${GOOD_CONTEXTS})
$(addprefix check.test.,${GOOD_CONTEXTS}): check.%: workdir.%
	diff -q $</cpt.incremental.en2fr ref/cpt.incremental.en2fr
	diff -q -r $</cpt.incremental.en2fr.tppt ref/cpt.incremental.en2fr.tppt
	diff -q $</lm.incremental_fr ref/lm.incremental_fr
	diff -q -r $</lm.incremental_fr.tplm ref/lm.incremental_fr.tplm
	diff -q $</canoe.ini.cow.orig $(<:workdir.test.%=%)/canoe.ini.cow
	diff -q $</soap-translate.sh $(<:workdir.test.%=%)/soap-translate.sh
	diff -q $</prime.sh $(<:workdir.test.%=%)/prime.sh
	diff -q $</cpt-incr.mixtm ref/$*/cpt-incr.mixtm
	diff -q $</lm-incr.mixlm ref/$*/lm-incr.mixlm
	diff -q $</incremental.config ref/$*/incremental.config
	diff -q $</canoe.ini.cow ref/$*/canoe.ini.cow
	configtool check $</canoe.ini.cow

workdir.test.context.1: EXTRA_INIT_OPTS =
workdir.test.context.2: EXTRA_INIT_OPTS = -lm-wt 0.2 -tm-wt 0.3
workdir.test.context.3: EXTRA_INIT_OPTS = -lm-wt 0.2 -tm-wt 0.1 0.2 0.3 0.4
workdir.test.context.4: EXTRA_INIT_OPTS =

$(addprefix workdir.test.,${GOOD_CONTEXTS}):  workdir.test.%: %
	mkdir -p $@
	cd $@ && ${INCR_INIT_MODEL} -v $(EXTRA_INIT_OPTS) -- ../$</canoe.ini.cow


.PHONY: $(addprefix check.test.,${BAD_CONTEXTS})
$(addprefix check.test.,${BAD_CONTEXTS}): check.%: workdir.%
	diff -q $</log.incr-init-model ref/$*/log.incr-init-model

$(addprefix workdir.test.,${BAD_CONTEXTS}):  workdir.test.%: %
	mkdir -p $@
	cd $@ && ! ${INCR_INIT_MODEL} -v -- ../$</canoe.ini.cow &> log.incr-init-model \
		&& echo "Failure for a bad context occurred as expected." >&2 \
		|| ! echo "ERROR: Failure was expected for a bad context, but it did not occur." >&2


test.existing_model: workdir.test.context.1
	cd $< && ${INCR_INIT_MODEL} -v ../${<:workdir.test.%=%}/canoe.ini.cow
