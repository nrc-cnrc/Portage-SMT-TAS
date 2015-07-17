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

TEMP_FILES := p.* json.* *.json
include ../Makefile.incl

SRC_DIR ?= src/
vpath q.dec ${SRC_DIR}
vpath %.raw ${SRC_DIR}
vpath %.tok ${SRC_DIR}

SHELL := bash

.DEFAULT_GOAL := all
.PHONY: all
all:  testcase




p.dec.oov.check  p.pal.check:  %.check:  %  ref/%
	diff  $+  --brief

p.json.check  p2.json.check  p3.json.check  json.args.tooShort.check  json.args.tooLong.check:  %.check:  %  ref/%
	diff  <(jq --sort-keys . $<)  <(jq --sort-keys . ref/$<)  --brief


testcase:  p.json.check
p.json:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-source=$(filter %q.dec,$+) \
		-json=$@ \
		< $(filter %p.raw,$+) \
	> /dev/null

testcase:  p2.json.check
p2.json:  Q2.tok  P2.tok  p2.raw
	${NBEST2RESCORE} \
		-wal \
		-source=$(filter %Q2.tok,$+) \
		-target=$(filter %P2.tok,$+) \
		-json=$@ \
		< $(filter %p2.raw,$+) \
	> /dev/null

# Validate using json without a user provided target.
testcase:  p3.json.check
p3.json:  Q2.tok  P2.tok  p2.raw
	${NBEST2RESCORE} \
		-wal \
		-source=$(filter %Q2.tok,$+) \
		-json=$@ \
		< $(filter %p2.raw,$+) \
	> /dev/null

# Test what happens when the target is not compatible with P.tok & p.raw.
testcase:  imcompatible.target.json
imcompatible.target.json:  Q2.tok  P2.tok  p2.raw
	${NBEST2RESCORE} \
		-wal \
		-source=$(filter %Q2.tok,$+) \
		-target=<(sed -e 's/Les/Les Les/' < $(filter %P2.tok,$+)) \
		-json=$@ \
		< $(filter %p2.raw,$+) \
	2>&1 \
	| grep "You provide a -target that doesn't match the input file."

# Are we correctly producing oov?
testcase:  p.dec.oov.check
p.dec.oov:  q.dec  p.raw
	${NBEST2RESCORE} \
		-tagoov \
		-oov \
		-wal \
		< $(filter %p.raw,$+) \
	| perl -pe 's/ +$$//;' \
	> $@

# Are we correctly producing pal?
testcase:  p.pal.check
p.pal:  q.dec  p.raw
	${NBEST2RESCORE} \
		-oov \
		-wal \
		-palout=p.pal \
		< $(filter %p.raw,$+) \
	> /dev/null

# Make sure the user provides -source when using -json or report an error.
testcase: json.args.error
json.args.error:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-json=$@ \
		< $(filter %p.raw,$+) 2>&1 \
	| grep --quiet 'Error: You must specify the source when using -json.'

# Validate that we detect when -source is to short.
testcase:  json.args.tooShort.check
json.args.tooShort:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-source=<(head -n 2 $(filter %q.dec,$+)) \
		-json=$@ \
		< $(filter %p.raw,$+) 2>&1 \
	| grep --quiet 'The source file is too short!'

# Validate that we detect when -source is to long.
testcase:  json.args.tooLong.check
json.args.tooLong:  q.dec  p.raw
	${NBEST2RESCORE} \
		-wal \
		-source=<(cat $(filter %q.dec,$+); echo "Extra to make it too long.") \
		-json=$@ \
		< $(filter %p.raw,$+) 2>&1 \
	| grep --quiet 'Source file too long!'

