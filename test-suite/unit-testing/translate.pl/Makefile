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


TEMP_FILES = U*.sdlxliff T*.tmx U*.sdlxliff.P.txt T*.tmx.P.txt log.*
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
	ln -fs $|/P.txt $@.P.txt


# Make sure we translated all sdlxliff examples first then try to validate them.
sdlxliff:  trans_sdlxliff

# Check an sdlxliff file
sdlxliff_check_%: plain_diff_% P_txt_diff_% tmx_wrap_check_%
	@:
plain_diff_%: % ref/ref.%
	diff $+ --brief
P_txt_diff_%: %.P.txt ref/ref.%.P.txt
	diff $+

# Translate only, no xtags, no CE
sdlxliff: validate_sdlxliff
validate_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
	@${MAKE} --no-print-directory sdlxliff_check_$<

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff:  override OPTIONS = -decode-only


# With CE
sdlxliff: validate_sdlxliff_CE
validate_sdlxliff_CE:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff
	@${MAKE} --no-print-directory sdlxliff_check_$<

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.CE.sdlxliff:  override OPTIONS = -with-ce


# With xtags
sdlxliff: validate_sdlxliff_xtags
validate_sdlxliff_xtags:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
	@${MAKE} --no-print-directory sdlxliff_check_$<

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff:  override OPTIONS = -decode-only -xtags


# With xtags and CE
sdlxliff: validate_sdlxliff_xtags_CE
validate_sdlxliff_xtags_CE:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff
	@${MAKE} --no-print-directory sdlxliff_check_$<

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.sdlxliff:  override OPTIONS = -with-ce -xtags


# With xtags and CE based filtering
sdlxliff: validate_sdlxliff_xtags_CE_filter
validate_sdlxliff_xtags_CE_filter:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff
	@${MAKE} --no-print-directory sdlxliff_check_$<

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.CE.filter.sdlxliff:  override OPTIONS = -with-ce -filter=0.2 -xtags



########################################
# TMX
.PHONY:  tmx  trans_tmx
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
	ln -fs $|/P.txt $@.P.txt


tmx:  trans_tmx

tmx_check_%: tmx_diff_% P_txt_diff_% tmx_wrap_check_%
	@:
tmx_diff_%: % ref/ref.%
	diff <(sed 's/changedate="[^"]*"//g' $*) <(sed 's/changedate="[^"]*"//g' ref/ref.$*) --brief
tmx_wrap_check_%: %
	! egrep '(open|close|tag)_wrap' $<


# Translate only, no xtags, no CE
tmx: validate_tmx
validate_tmx: TestMemory_Unknown.translated.tmx
	@${MAKE} --no-print-directory tmx_check_$<

trans_tmx:  TestMemory_Unknown.translated.tmx
TestMemory_Unknown.translated.tmx:  override OPTIONS = -decode-only


# With CE
tmx: validate_tmx_CE
validate_tmx_CE: TestMemory_Unknown.CE.tmx
	@${MAKE} --no-print-directory tmx_check_$<

trans_tmx:  TestMemory_Unknown.CE.tmx
TestMemory_Unknown.CE.tmx:  override OPTIONS = -with-ce


# With xtags
tmx: validate_tmx_xtags
validate_tmx_xtags: TestMemory_Unknown.xtags.tmx
	@${MAKE} --no-print-directory tmx_check_$<

trans_tmx:  TestMemory_Unknown.xtags.tmx
TestMemory_Unknown.xtags.tmx:  override OPTIONS = -decode-only -xtags


# With xtags and CE
tmx: validate_tmx_xtags_CE
validate_tmx_xtags_CE: TestMemory_Unknown.xtags.CE.tmx
	@${MAKE} --no-print-directory tmx_check_$<

trans_tmx:  TestMemory_Unknown.xtags.CE.tmx
TestMemory_Unknown.xtags.CE.tmx:  override OPTIONS = -with-ce -xtags


# With xtags and CE-based filtering
tmx: validate_tmx_xtags_CE_filter
validate_tmx_xtags_CE_filter: TestMemory_Unknown.xtags.CE.filter.tmx
	@${MAKE} --no-print-directory tmx_check_$<

trans_tmx:  TestMemory_Unknown.xtags.CE.filter.tmx
TestMemory_Unknown.xtags.CE.filter.tmx:  override OPTIONS = -with-ce -xtags -filter=0.20




#: UnknownSegments_en-CA_fr-CA.sdlxliff
#/opt/Portage/bin/translate.pl \
#   -src=en \
#   -tgt=fr \
#   -xsrc=EN-CA \
#   -xtgt=FR-CA \
#   -tctp \
#   -f=/opt/Portage/models/toy.en2fr/canoe.ini.cow \
#   -verbose \
#   -out=/var/www/html/plive/CE_toy.en2fr_test_numbers_hyphens_2.docx.sdlxliff_20130109T141704Z_IVctCi/P.out \
#   -dir=/var/www/html/plive/CE_toy.en2fr_test_numbers_hyphens_2.docx.sdlxliff_20130109T141704Z_IVctCi \
#   -decode-only \
#   -xml \
#   -nl=s \
#   -xtags \
#   /var/www/html/plive/CE_toy.en2fr_test_numbers_hyphens_2.docx.sdlxliff_20130109T141704Z_IVctCi/Q.in
