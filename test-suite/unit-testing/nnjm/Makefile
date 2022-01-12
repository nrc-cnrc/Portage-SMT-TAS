#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief test out nnjm with canoe.
#
# @author Colin Cherry
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

CANOE := canoe
SHELL := /bin/bash

TEMP_DIRS=out out2
TEMP_FILES= *~ canoe.ini.mm
include ../Makefile.incl

all: cmp_out/log cmp_out/log.noal
all: cmp_out2/log cmp_out2/log.noal
all: memmap

out:
	mkdir -p out

cmp_out/%: out out/%
	egrep '^(NNJM|Warning):' out/$* | diff - ref/$* -q

out/log:
	${CANOE} -f models/canoe.ini -srctags corpus/train_fr.tag < corpus/train_fr.tok >& $@

out/log.noal:
	${CANOE} -f models/canoe.noal.ini -srctags corpus/train_fr.tag < corpus/train_fr.tok >& $@

out2:
	mkdir -p out2

cmp_out2/%: out2 out2/%
	egrep '^(NNJM|Warning):' out2/$* | sed 's/nnjm2/nnjm/' | diff - ref/$* -q

out2/log:
	${CANOE} -f models/canoe.ini -nnjm-file models/nnjm2 < corpus/train_fr.tok >& $@

out2/log.noal:
	${CANOE} -f models/canoe.noal.ini -nnjm-file models/nnjm2 < corpus/train_fr.tok >& $@

ifdef PORTAGE_GENERIC_MODEL
ifneq ($(wildcard $(PORTAGE_GENERIC_MODEL)/generic-2.1/*),)
canoe.ini.mm: models/canoe.ini
	configtool args:"-nnjm-file $$PORTAGE_GENERIC_MODEL/generic-2.1/nnjm/nnjm.generic-2.1.fr2en/model" $< $@

memmap: canoe.ini.mm
	M=`configtool memmap $<`; echo $$M; if [[ $$M < 10 ]]; then echo "configtool memmap is not including NNJM: got $$M, expected 200+"; exit 1; fi
else
memmap:
	@echo "Warning: Skipping NNJM memmap test since $(PORTAGE_GENERIC_MODEL)/generic-2.1 is not found"
endif
else
memmap:
	@echo "Warning: Skipping NNJM memmap test since PORTAGE_GENERIC_MODEL is not defined"
endif
