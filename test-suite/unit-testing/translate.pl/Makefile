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
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

SOAP_TRANSLATE := $$PORTAGE/test-suite/systems/toy-regress-en2fr/soap-translate.sh

-include Makefile.params

vpath %.sdlxliff ../../data/sdlxliff
vpath %.tmx ../../data/tmx
vpath ref.% ref

SHELL = bash

.SUFFIXES:
.SECONDARY:
.DEFAULT_GOAL := all
.PHONY: all
all:  sdlxliff
all:  tmx
all:  other_tests


TEMP_FILES = U*.sdlxliff T*.tmx U*.sdlxliff.P.txt T*.tmx.P.txt log.* out.*
TEMP_DIRS = workdir.* translate_work_*
include ../Makefile.incl


workdir.%:
	mkdir -p $@


# Only performs translation.
.PHONY:  trans
trans:  trans_sdlxliff
trans:  trans_tmx

.PHONY:  decode_only

########################################
# SDLXLIFF
.PHONY:  sdlxliff  trans_sdlxliff
UnknownSegments_en-CA_fr-CA.v5.%.sdlxliff.P.txt:  UnknownSegments_en-CA_fr-CA.v5.%.sdlxliff
	ln -fs workdir.$*.sdlxliff/P.txt $@

UnknownSegments_en-CA_fr-CA.v5.%.sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.sdlxliff  | workdir.%.sdlxliff
	${SOAP_TRANSLATE} \
		-verbose \
		-out=$|/P.out \
		-dir=$| \
		-nl=s \
		-xml \
		${OPTIONS} \
		$< \
		2> log.$@
	ln -fs $|/QP.xml $@


# Make sure we translated all sdlxliff examples first then try to validate them.
sdlxliff:  trans_sdlxliff

# Check an sdlxliff file
%.sdlxliff.validate:  %.sdlxliff.diff  %.sdlxliff.txt_diff  %.sdlxliff.wrap
	@:
%.sdlxliff.diff:  %.sdlxliff  ref/ref.%.sdlxliff
	diff <( \
	   sed -e 's/="Portage[^"]*"/="PortageXYZ"/g' \
	       -e 's/tool-version="[^"]*"/tool-version="y.z"/g' \
	       $*.sdlxliff \
	   ) ref/ref.$*.sdlxliff --brief
%.txt_diff:  %.P.txt  ref/ref.%.P.txt
	diff $+


# Translate only, no xtags, no CE
sdlxliff: validate_sdlxliff
validate_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff.validate
	@true

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
decode_only:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff:  override OPTIONS = -decode-only


# With CE
sdlxliff: validate_sdlxliff_CE
validate_sdlxliff_CE:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff.validate
	@true

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff:  override OPTIONS = -with-ce


# With xtags
sdlxliff: validate_sdlxliff_xtags
validate_sdlxliff_xtags:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff.validate
	@true

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff:  override OPTIONS = -decode-only -xtags


# With xtags and CE
sdlxliff: validate_sdlxliff_xtags_CE
validate_sdlxliff_xtags_CE:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff.validate
	@true

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff:  override OPTIONS = -with-ce -xtags


# With xtags and CE based filtering
sdlxliff: validate_sdlxliff_xtags_CE_filter
validate_sdlxliff_xtags_CE_filter:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff.validate
	@true

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff:  override OPTIONS = -with-ce -filter=0.2 -xtags



########################################
# TMX
.PHONY:  tmx  trans_tmx
TestMemory_Unknown.%.tmx.P.txt:  TestMemory_Unknown.%.tmx
	ln -fs workdir.$*.tmx/P.txt $@

TestMemory_Unknown.%.tmx:  TestMemory_Unknown.tmx  | workdir.%.tmx
	${SOAP_TRANSLATE} \
		-verbose \
		-out=$|/P.out \
		-dir=$| \
		-nl=s \
		-xml \
		${OPTIONS} \
		$< \
		2> log.$@
	ln -fs $|/QP.xml $@


tmx:  trans_tmx

%.tmx.validate:  %.tmx.diff  %.tmx.txt_diff  %.tmx.wrap
	@:
%.tmx.diff:  %.tmx  ref/ref.%.tmx
	diff <(sed 's/changedate="[^"]*"//g' $*.tmx) <(sed 's/changedate="[^"]*"//g' ref/ref.$*.tmx) --brief
%.wrap:  %
	! egrep '(open|close|tag)_wrap' $<


# Translate only, no xtags, no CE
tmx: validate_tmx
validate_tmx: TestMemory_Unknown.translated.tmx.validate
	@true

trans_tmx:  TestMemory_Unknown.translated.tmx
decode_only:  TestMemory_Unknown.translated.tmx
TestMemory_Unknown.translated.tmx:  override OPTIONS = -decode-only


# With CE
tmx: validate_tmx_CE
validate_tmx_CE: TestMemory_Unknown.CE.tmx.validate
	@true

trans_tmx:  TestMemory_Unknown.CE.tmx
TestMemory_Unknown.CE.tmx:  override OPTIONS = -with-ce


# With xtags
tmx: validate_tmx_xtags
validate_tmx_xtags: TestMemory_Unknown.xtags.tmx.validate
	@true

trans_tmx:  TestMemory_Unknown.xtags.tmx
TestMemory_Unknown.xtags.tmx:  override OPTIONS = -decode-only -xtags


# With xtags and CE
tmx: validate_tmx_xtags_CE
validate_tmx_xtags_CE: TestMemory_Unknown.xtags.CE.tmx.validate
	@true

trans_tmx:  TestMemory_Unknown.xtags.CE.tmx
TestMemory_Unknown.xtags.CE.tmx:  override OPTIONS = -with-ce -xtags


# With xtags and CE-based filtering
tmx: validate_tmx_xtags_CE_filter
validate_tmx_xtags_CE_filter: TestMemory_Unknown.xtags.CE.filter.tmx.validate
	@true

trans_tmx:  TestMemory_Unknown.xtags.CE.filter.tmx
TestMemory_Unknown.xtags.CE.filter.tmx:  override OPTIONS = -with-ce -xtags -filter=0.20



########################################
# Additional tests
.PHONY:  other_tests

# Check handling of bad locale (i.e. bad country code)
.PHONY:  bad_locale
other_tests: bad_locale
bad_locale:  TestMemory_Unknown.tmx  | workdir.bad_locale.tmx
	! ${SOAP_TRANSLATE} \
		-verbose \
		-out=$|/P.out \
		-dir=$| \
		-nl=s \
		-xml \
		-decode-only \
		-src-country=XX \
		$< \
		2> log.$@ \
	|| ! echo "Error: invalid locale not detected."
	grep "Error: Invalid locale" log.$@ || ! echo "Error: invalid locale not detected."

# Check that we not longer pointlessly report on empty input lines (PTGSH-347)
.PHONY:  empty_input_lines
other_tests: empty_input_lines
empty_input_lines: paragraph-in
	${SOAP_TRANSLATE} -decode-only -nl=p $< > out.$@ 2> log.$@
	! grep Empty log.$@
