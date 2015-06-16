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
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

NBEST2RESCORE := nbest2rescore.pl

-include Makefile.params

TEMP_FILES := p.* json.*
include ../Makefile.incl

SRC_DIR ?= src/
vpath q.dec ${SRC_DIR}
vpath p.raw ${SRC_DIR}

SHELL := bash

.DEFAULT_GOAL := all
.PHONY: all
all:  testcase




p.dec.oov.check  p.pal.check:  %.check:  %  ref/%
	diff  $+  --brief

p.json.check  json.args.tooShort.check  json.args.tooLong.check:  %.check:  %  ref/%
	diff  <(jq --sort-keys . $<)  <(jq --sort-keys . ref/$<)  --brief


testcase:  p.json.check
p.json:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-source=$(filter %q.dec,$+) \
		-json=$@ \
		< $(filter %p.raw,$+) \
	> /dev/null

testcase:  p.dec.oov.check
p.dec.oov:  q.dec  p.raw
	${NBEST2RESCORE} \
		-tagoov \
		-oov \
		-wal \
		< $(filter %p.raw,$+) \
	| perl -pe 's/ +$$//;' \
	> $@

testcase:  p.pal.check
p.pal:  q.dec  p.raw
	${NBEST2RESCORE} \
		-oov \
		-wal \
		-palout=p.pal \
		< $(filter %p.raw,$+) \
	> /dev/null

testcase: json.args.error
json.args.error:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-json=$@ \
		< $(filter %p.raw,$+) 2>&1 \
	| grep --quiet 'Error: You must specify the source when using -json.'

testcase:  json.args.tooShort.check
json.args.tooShort:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-source=<(head -n 2 $(filter %q.dec,$+)) \
		-json=$@ \
		< $(filter %p.raw,$+) 2>&1 \
	| grep --quiet 'The source file is too short!'

testcase:  json.args.tooLong.check
json.args.tooLong:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-source=<(cat $(filter %q.dec,$+); echo "Extra to make it too long.") \
		-json=$@ \
		< $(filter %p.raw,$+) 2>&1 \
	| grep --quiet 'Source file too long!'

