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
	../../data/toy-regress/soap-translate.sh \
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

sdlxliff: validate_sdlxliff
validate_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.sdlxliff
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $< <(sed -e 's#<\/\?g[^>]*>##g' $(filter ref/%, $+)) --brief

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.translated.sdlxliff:  override OPTIONS = -decode-only


sdlxliff: validate_sdlxliff_tags
validate_sdlxliff_tags:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff:  override OPTIONS = -decode-only -xtags


sdlxliff: validate_sdlxliff_tags_CE
validate_sdlxliff_tags_CE:  UnknownSegments_en-CA_fr-CA.v5.xtags.ce.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief


trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.ce.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.ce.sdlxliff:  override OPTIONS = -with-ce -xtags


sdlxliff: validate_sdlxliff_tags_CE_filter
validate_sdlxliff_tags_CE_filter:  UnknownSegments_en-CA_fr-CA.v5.xtags.ce.filter.sdlxliff  ref.UnknownSegments_en-CA_fr-CA.v5.xtags.sdlxliff
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief

trans_sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.xtags.ce.filter.sdlxliff
UnknownSegments_en-CA_fr-CA.v5.xtags.ce.filter.sdlxliff:  override OPTIONS = -with-ce -filter=0.2 -xtags



########################################
# TMX
.PHONY:  tmx  trans_tmx
TestMemory_Unknown.%.tmx:  TestMemory_Unknown.tmx  | workdir.%.tmx
	../../data/toy-regress/soap-translate.sh \
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

tmx: validate_tmx
validate_tmx: TestMemory_Unknown.translated.tmx ref.TestMemory_Unknown.tmx
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief

trans_tmx:  TestMemory_Unknown.translated.tmx
TestMemory_Unknown.translated.tmx:  override OPTIONS = -decode-only


tmx: validate_tmx_xtags
validate_tmx_xtags: TestMemory_Unknown.xtags.tmx ref.TestMemory_Unknown.xtags.tmx
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief

trans_tmx:  TestMemory_Unknown.xtags.tmx
TestMemory_Unknown.xtags.tmx:  override OPTIONS = -decode-only -xtags


tmx: validate_tmx_xtags_CE
validate_tmx_xtags_CE: TestMemory_Unknown.xtags.ce.tmx ref.TestMemory_Unknown.xtags.tmx
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief

trans_tmx:  TestMemory_Unknown.xtags.ce.tmx
TestMemory_Unknown.xtags.ce.tmx:  override OPTIONS = -with-ce -xtags


tmx: validate_tmx_xtags_CE_filter
validate_tmx_xtags_CE_filter: TestMemory_Unknown.xtags.ce.filter.tmx ref.TestMemory_Unknown.xtags.tmx
	! egrep '(open|clsoe|tag)_wrap' $<
	diff $+ --brief

trans_tmx:  TestMemory_Unknown.xtags.ce.filter.tmx
TestMemory_Unknown.xtags.ce.filter.tmx:  override OPTIONS = -with-ce -xtags -filter=0.27




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
