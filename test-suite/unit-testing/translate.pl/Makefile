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

.DEFAULT_GOAL := all
.PHONY: all
all:


.PHONY: clean
clean:

workdir:
	@mkdir $@

test:  UnknownSegments_en-CA_fr-CA.sdlxliff  | workdir
	../../data/toy-regress/soap-translate.sh \
		-verbose \
		-out=$|/P.out \
		-dir=$| \
		-decode-only \
		-nl=s \
		-xml \
		-xtags \
		$<

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
