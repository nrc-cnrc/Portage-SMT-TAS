#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file Makefile
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

SERVER := iitfrdvm023110
CONTEXT := toy-regress-afield

-include Makefile.params

vpath %.sdlxliff ../../data/sdlxliff
vpath %.tmx ../../data/tmx
vpath ref.% ../translate.pl/ref

SHELL = bash

.SUFFIXES:
.SECONDARY:
.NOTPARALLEL:
.DEFAULT_GOAL := all
.PHONY: all
$(info SERVER: $(word 1,$(subst :, ,${SERVER})))
ifeq ($(shell ping -c 1 $(word 1,$(subst :, ,${SERVER})) &> /dev/null && echo TRUE),TRUE)
# Performs this unittest only if the server is reachable.
all:  sdlxliff
all:  tmx
else
all:
endif


TEMP_FILES = U*.sdlxliff T*.tmx log.*
TEMP_DIRS = workdir.*
include ../Makefile.incl


workdir.%:
	mkdir -p $@


# Only performs translation.
.PHONY:  trans
trans:  trans_sdlxliff
trans:  trans_tmx

########################################
# SDLXLIFF
.PHONY:  sdlxliff  trans_sdlxliff
UnknownSegments_en-CA_fr-CA.v5.%.sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.sdlxliff  | workdir.%.sdlxliff
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=${CONTEXT} \
		-file=$< \
		${OPTIONS} \
		2> log.$@ \
		> $@


# Make sure we translated all sdlxliff examples first then try to validate them.
sdlxliff:  trans_sdlxliff

validate_sdlxlif%:
	! egrep '(open|close|tag)_wrap' $<
	diff $+ --brief


# Translate only, no xtags, no CE
sdlxliff: validate_sdlxliff
#validate_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.sdlxliff
validate_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff:  override OPTIONS =


# With CE
sdlxliff: validate_sdlxliff_CE
validate_sdlxliff_CE:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff:  override OPTIONS = -filter=0.0


# With xtags
sdlxliff: validate_sdlxliff_xtags
#validate_sdlxliff_xtags:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
validate_sdlxliff_xtags:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff:  override OPTIONS = -xtags


# With xtags and CE
sdlxliff: validate_sdlxliff_xtags_CE
validate_sdlxliff_xtags_CE:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff:  override OPTIONS = -filter=0.0 -xtags


# With xtags and CE based filtering
sdlxliff: validate_sdlxliff_xtags_CE_filter
validate_sdlxliff_xtags_CE_filter:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff:  override OPTIONS = -filter=0.2 -xtags



########################################
# TMX
.PHONY:  tmx  trans_tmx
TestMemory_Unknown.%.tmx:  TestMemory_Unknown.tmx  | workdir.%.tmx
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=${CONTEXT} \
		-file=$< \
		${OPTIONS} \
		2> log.$@ \
		> $@


tmx:  trans_tmx

validate_tm%:
	! egrep '(open|close|tag)_wrap' $<
	diff <(sed 's/changedate="[^"]*"//g' $<) <(sed 's/changedate="[^"]*"//g' $(filter-out $<,$+)) --brief


# Translate only, no xtags, no CE
tmx: validate_tmx
validate_tmx: TestMemory_Unknown.translated.tmx ref.TestMemory_Unknown.CE.tmx

trans_tmx:  TestMemory_Unknown.translated.tmx
TestMemory_Unknown.translated.tmx:  override OPTIONS =


# With CE
tmx: validate_tmx_CE
validate_tmx_CE: TestMemory_Unknown.CE.tmx ref.TestMemory_Unknown.CE.tmx

trans_tmx:  TestMemory_Unknown.CE.tmx
TestMemory_Unknown.CE.tmx:  override OPTIONS = -filter=0.0


# With xtags
tmx: validate_tmx_xtags
validate_tmx_xtags: TestMemory_Unknown.xtags.tmx ref.TestMemory_Unknown.xtags.CE.tmx

trans_tmx:  TestMemory_Unknown.xtags.tmx
TestMemory_Unknown.xtags.tmx:  override OPTIONS = -xtags


# With xtags and CE
tmx: validate_tmx_xtags_CE
validate_tmx_xtags_CE: TestMemory_Unknown.xtags.CE.tmx ref.TestMemory_Unknown.xtags.CE.tmx

trans_tmx:  TestMemory_Unknown.xtags.CE.tmx
TestMemory_Unknown.xtags.CE.tmx:  override OPTIONS = -filter=0.0 -xtags


# With xtags and CE-based filtering
tmx: validate_tmx_xtags_CE_filter
validate_tmx_xtags_CE_filter: TestMemory_Unknown.xtags.CE.filter.tmx ref.TestMemory_Unknown.xtags.CE.filter.tmx

trans_tmx:  TestMemory_Unknown.xtags.CE.filter.tmx
TestMemory_Unknown.xtags.CE.filter.tmx:  override OPTIONS = -xtags -filter=0.20

