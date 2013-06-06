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

TEMP_FILES = *.sdlxliff *.tmx log.*
include ../Makefile.incl

$(info SERVER: $(word 1,$(subst :, ,${SERVER})))
ifneq ($(shell ping -c 1 $(word 1,$(subst :, ,${SERVER})) &> /dev/null && echo TRUE),TRUE)
all: all_no_server

.PHONY: all_no_server
all_no_server extra:
	@echo "Server not available, so PortageLiveAPI unittests were NOT run."
else
# Performs this unittest only if the server is reachable.
all:  sdlxliff
all:  tmx



# Only performs translation.
.PHONY:  trans
trans:  trans_sdlxliff
trans:  trans_tmx

########################################
# SDLXLIFF
.PHONY:  sdlxliff  trans_sdlxliff
UnknownSegments_en-CA_fr-CA.v5.%.sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.sdlxliff
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=${CONTEXT} \
		-file=$< \
		${OPTIONS} \
		2> log.$@ \
		> $@


# Make sure we translated all sdlxliff examples first then try to validate them.
sdlxliff:  trans_sdlxliff

%.sdlxliff.validate:  %.sdlxliff.wrap  %.sdlxliff.diff
	@:
%.sdlxliff.diff:  %.sdlxliff
	diff $+ --brief
%.wrap:  %
	! egrep '(open|close|tag)_wrap' $<


# Translate only, no xtags, no CE
sdlxliff: validate_sdlxliff
validate_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff.validate
UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff.diff:  ref.UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff:  override OPTIONS =


# With CE
sdlxliff: validate_sdlxliff_CE
validate_sdlxliff_CE:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff.validate
UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff.diff:  ref.UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff:  override OPTIONS = -filter=0.0


# With xtags
sdlxliff: validate_sdlxliff_xtags
validate_sdlxliff_xtags:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff.validate
UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff.diff:  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff:  override OPTIONS = -xtags


# With xtags and CE
sdlxliff: validate_sdlxliff_xtags_CE
validate_sdlxliff_xtags_CE:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff.validate
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff.diff:  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff:  override OPTIONS = -filter=0.0 -xtags


# With xtags and CE based filtering
sdlxliff: validate_sdlxliff_xtags_CE_filter
validate_sdlxliff_xtags_CE_filter:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff.validate
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff.diff:  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff:  override OPTIONS = -filter=0.2 -xtags



########################################
# TMX
.PHONY:  tmx  trans_tmx
TestMemory_Unknown.%.tmx:  TestMemory_Unknown.tmx
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=${CONTEXT} \
		-file=$< \
		${OPTIONS} \
		2> log.$@ \
		> $@


tmx:  trans_tmx

%.tmx.validate:  %.tmx.diff  %.tmx.wrap
	@:
%.tmx.diff:  %.tmx
	diff <(sed 's/changedate="[^"]*"//g' $<) <(sed 's/changedate="[^"]*"//g' $(filter-out $<,$+)) --brief


# Translate only, no xtags, no CE
tmx: validate_tmx
validate_tmx: TestMemory_Unknown.translated.tmx.validate
TestMemory_Unknown.translated.tmx.diff:  ref.TestMemory_Unknown.CE.tmx

trans_tmx:  TestMemory_Unknown.translated.tmx
TestMemory_Unknown.translated.tmx:  override OPTIONS =


# With CE
tmx: validate_tmx_CE
validate_tmx_CE: TestMemory_Unknown.CE.tmx.validate
TestMemory_Unknown.CE.tmx.diff:  ref.TestMemory_Unknown.CE.tmx

trans_tmx:  TestMemory_Unknown.CE.tmx
TestMemory_Unknown.CE.tmx:  override OPTIONS = -filter=0.0


# With xtags
tmx: validate_tmx_xtags
validate_tmx_xtags: TestMemory_Unknown.xtags.tmx.validate
TestMemory_Unknown.xtags.tmx.diff:  ref.TestMemory_Unknown.xtags.CE.tmx

trans_tmx:  TestMemory_Unknown.xtags.tmx
TestMemory_Unknown.xtags.tmx:  override OPTIONS = -xtags


# With xtags and CE
tmx: validate_tmx_xtags_CE
validate_tmx_xtags_CE: TestMemory_Unknown.xtags.CE.tmx.validate
TestMemory_Unknown.xtags.CE.tmx.diff:  ref.TestMemory_Unknown.xtags.CE.tmx

trans_tmx:  TestMemory_Unknown.xtags.CE.tmx
TestMemory_Unknown.xtags.CE.tmx:  override OPTIONS = -filter=0.0 -xtags


# With xtags and CE-based filtering
tmx: validate_tmx_xtags_CE_filter
validate_tmx_xtags_CE_filter: TestMemory_Unknown.xtags.CE.filter.tmx.validate
TestMemory_Unknown.xtags.CE.filter.tmx.diff:  ref.TestMemory_Unknown.xtags.CE.filter.tmx

trans_tmx:  TestMemory_Unknown.xtags.CE.filter.tmx
TestMemory_Unknown.xtags.CE.filter.tmx:  override OPTIONS = -xtags -filter=0.20







################################################################################
# EXTRAS
# translate with EC or EC-walh arbitrary tmx & sdlxliff input files.
LOGS ?= .logs/
$(shell mkdir -p ${LOGS})

EXTRA_SOURCE_DIR ?= /some/path/where/to/get/extra/inputs

TMX      := $(notdir $(wildcard ${EXTRA_SOURCE_DIR}/TMX/*.tmx))
SDLXLIFF := $(notdir $(wildcard ${EXTRA_SOURCE_DIR}/XLIFF/*.sdlxliff))

vpath %.tmx ${EXTRA_SOURCE_DIR}/TMX
vpath %.sdlxliff ${EXTRA_SOURCE_DIR}/XLIFF

.PHONY:  extra
extra:  ${TMX:.tmx=.EC.xtags.CE.filter.tmx.e_validate}
extra:  ${TMX:.tmx=.EC-walh.xtags.CE.filter.tmx.e_validate}
extra:  ${SDLXLIFF:.sdlxliff=.EC.xtags.CE.filter.sdlxliff.e_validate}
extra:  ${SDLXLIFF:.sdlxliff=.EC-walh.xtags.CE.filter.sdlxliff.e_validate}

# What do we need to validate for extra sdlxliff files?
%.sdlxliff.e_validate:  %.sdlxliff.wrap  %.sdlxliff.tool_name  %.sdlxliff.target  %.sdlxliff.origin  %.sdlxliff.e_diff
	@:
# Finding the tool-company to be CNRC-NRC let us believe that the input file was processed by Portage.
%.sdlxliff.tool_name:  %.sdlxliff
	grep 'tool-company="CNRC-NRC"' $<  --quiet
# In a non-degenerative case we want to find a target which indicates a translation.
%.sdlxliff.target:  %.sdlxliff
	grep '<target>' $<  --quiet
# Finding the origin to be mt would alsindicate that Portage was involved.
%.sdlxliff.origin:  %.sdlxliff
	grep 'origin="mt"' $< --quiet

# What do we need to validate for extra tmx files?
%.tmx.e_validate:  %.tmx.wrap  %.tmx.id  %.tmx.CE  %.tmx.e_diff
	@:
# Finding the changeid to be MT! would indicate that Portage was involved.
%.tmx.id:  %.tmx
	grep 'changeid="MT!"' $< --quiet
# NOTE: This is not the safest test since we are filtering and in a
# degenerative case, all translations would be empty with no CE.
%.tmx.CE:  %.tmx
	grep '<prop type="Txt::CE">' $< --quiet


# The translation output file must not be the same as the input file.
${TMX:.tmx=.EC.xtags.CE.filter.tmx.e_diff}:  %.EC.xtags.CE.filter.tmx.e_diff:  %.tmx
${TMX:.tmx=.EC-walh.xtags.CE.filter.tmx.e_diff}:  %.EC-walh.xtags.CE.filter.tmx.e_diff:  %.tmx
${SDLXLIFF:.sdlxliff=.EC.xtags.CE.filter.sdlxliff.e_diff}:  %.EC.xtags.CE.filter.sdlxliff.e_diff:  %.sdlxliff
${SDLXLIFF:.sdlxliff=.EC-walh.xtags.CE.filter.sdlxliff.e_diff}:  %.EC-walh.xtags.CE.filter.sdlxliff.e_diff:  %.sdlxliff
# Requires the translation output file and the input file.
%.e_diff:  %
	! diff $+ --brief


FILTER ?= -filter=0.20
%.EC.xtags.CE.filter.sdlxliff:  %.sdlxliff
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=EC \
		-file=$< \
		-xtags ${FILTER} \
		2> ${LOGS}log.$@ \
		> $@

%.EC-walh.xtags.CE.filter.sdlxliff:  %.sdlxliff
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=EC-walh \
		-file=$< \
		-xtags ${FILTER} \
		2> ${LOGS}log.$@ \
		> $@

%.EC.xtags.CE.filter.tmx:  %.tmx
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=EC \
		-file=$< \
		-xtags ${FILTER} \
		2> ${LOGS}log.$@ \
		> $@

%.EC-walh.xtags.CE.filter.tmx:  %.tmx
	PortageLiveAPI.pl \
		-server=${SERVER} \
		-context=EC-walh \
		-file=$< \
		-xtags ${FILTER} \
		2> ${LOGS}log.$@ \
		> $@

endif
