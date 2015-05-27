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

TEMP_DIRS=out
TEMP_FILES= *~
include ../Makefile.incl

all: cmp_out/log cmp_out/log.noal

out:
	mkdir -p out

cmp_out/%: out out/%
	egrep '^(NNJM|Warning):' out/$* | diff - ref/$* -q

out/log:
	${CANOE} -f models/canoe.ini -srctags corpus/train_fr.tag < corpus/train_fr.tok >& out/log

out/log.noal:
	${CANOE} -f models/canoe.noal.ini -srctags corpus/train_fr.tag < corpus/train_fr.tok >& out/log.noal
