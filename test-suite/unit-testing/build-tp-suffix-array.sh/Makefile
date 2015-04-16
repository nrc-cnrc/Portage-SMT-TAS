#!/usr/bin/make -f
# vim:noet:ts=3

SHELL=bash

VOCAB.BUILD ?= vocab.build
#VOCAB.BUILD = ../../../src/tpt/vocab.build

VOCAB.DUMP ?= vocab.dump
#VOCAB.DUMP = ../../../src/tpt/vocab.dump

TOKENINDEX.DUMP ?= tokenindex.dump
#TOKENINDEX.DUMP = ../../../src/tpt/tokenindex.dump

MMCTRACK.BUILD ?= mmctrack.build
#MMCTRACK.BUILD = ../../../src/tpt/mmctrack.build

MMCTRACK.DUMP ?= mmctrack.dump
#MMCTRACK.DUMP = ../../../src/tpt/mmctrack.dump

MMSUFA.BUILD ?= mmsufa.build
#MMSUFA.BUILD = ../../../src/tpt/mmsufa.build

MMSUFA.DUMP ?= mmsufa.dump
#MMSUFA.DUMP = ../../../src/tpt/mmsufa.dump

BUILD_TP_SUFFIX_ARRAY ?= build-tp-suffix-array.sh
#BUILD_TP_SUFFIX_ARRAY = ../../../src/tpt/build-tp-suffix-array.sh

FIND_SIMILAR_SENTENCES ?= find_similar_sentences
#FIND_SIMILAR_SENTENCES = ../../../src/tpt/find_similar_sentences

.PHONY: all
all: test


TEMP_FILES=test.vcb test.tdx test.mct test.msa similar.txt small.* log.*
TEMP_DIRS=test_en.tpsa *.tpsa.tmp.*
include ../Makefile.incl


.PHONY: test test1 test2 test3 test4 test5
# The following commands are executed by "build-tp-suffix-array.sh test_en.al test":
#   vocab.build --tdx test.tdx <test_en.al
#   mmctrack.build test.tdx test.mct <test_en.al
#   mmsufa.build test.mct test.msa
test: test1 test2 test3 test4 test5 testsampling

########################################
# Builds a vocabulary & a token index.
test.vcb test.tdx:  test_en.al
	${VOCAB.BUILD} test.vcb --tdx test.tdx < test_en.al #>&log.vocab.build

test1: test.vcb test.tdx
	${VOCAB.DUMP} test.vcb | cut -c11-48 | diff-round.pl -sort -q - ref/test.vcb.txt
	${TOKENINDEX.DUMP} test.tdx | sed 's/ *[0-9]* //' | li-sort.sh | diff -b - ref/test.tdx.txt -q


########################################
# Builds a memory mapped corpus track.
test.mct:  test.tdx test_en.al
	${MMCTRACK.BUILD} test.tdx test.mct < test_en.al

test2:  test.tdx test.mct
	${MMCTRACK.DUMP} test.tdx test.mct | diff -b - test_en.al -q


########################################
# Builds a memory mapped suffix array.
%.msa:  %.mct
	${MMSUFA.BUILD} $+ $@

test3:  test.tdx test.mct test.msa
	${MMSUFA.DUMP} test.tdx test.mct test.msa | li-sort.sh | diff - ref/test.msa.txt -q


########################################
# Using build-tp-suffix-array.sh that creates a token index, memory mapped corpus track & memory mapped suffix array.
test4:
	${BUILD_TP_SUFFIX_ARRAY} -v test_en.al.gz test_en &> log.build-tp-suffix-array.sh
	${TOKENINDEX.DUMP} test_en.tpsa/tdx | sed 's/ *[0-9]* //' | li-sort.sh | diff -b - ref/test.tdx.txt -q
	${MMCTRACK.DUMP} test_en.tpsa/tdx test_en.tpsa/mct | diff -b - test_en.al -q
	${MMSUFA.DUMP} test_en.tpsa/tdx test_en.tpsa/mct test_en.tpsa/msa | li-sort.sh | diff - ref/test.msa.txt -q


########################################
# 
test5:  test.tdx test.mct test.msa
	${FIND_SIMILAR_SENTENCES} -s 0.1 test < similar_en.al >similar.txt
	diff-round.pl -q similar.txt ref/similar.txt


.PHONY: small_test small_test1 small_test2 small_test3
small_test: small_test1 small_test2 small_test3

small_test1:
	${VOCAB.BUILD} small.vcb --tdx small.tdx <small_test_en.al
	${VOCAB.DUMP} small.vcb >small.vcb.txt
	${TOKENINDEX.DUMP} small.tdx >small.tdx.txt

small_test2:
	${MMCTRACK.BUILD} small.tdx small.mct <small_test_en.al
	${MMCTRACK.DUMP} small.tdx small.mct >small.mct.txt

small_test3:
	${MMSUFA.BUILD} small.mct small.msa
	${MMSUFA.DUMP} small.tdx small.mct small.msa >small.msa.txt


########################################
# Sampling test
testsampling: test4
	diff <(mmtsa.find --tpsa test_en.tpsa action | sort) \
	     <(mmtsa.find --tpsa test_en.tpsa --random action | sort)
