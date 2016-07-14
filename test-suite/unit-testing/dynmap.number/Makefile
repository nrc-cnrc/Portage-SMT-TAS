#!/usr/bin/make -f
# This makefile runs the toy example
# vim:ts=3:noet
#
# Samuel Larkin
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

MAP_TYPE ?= prefixNumber

all: dynmap_visual_test dynmap_cmp

toy.pdf: toy.tex
	pdflatex -interaction=batchmode toy.tex
	pdflatex -interaction=batchmode toy.tex

doc: toy.pdf

.PHONY: clean tokenize align lowercase split lm tm cow rat translate rescore test all

PORTAGE ?= /home/portage
#SOURCE_DIR ?= ${PORTAGE}/test-suite/toy
SOURCE_DIR ?= ../toy
vpath hans%.txt ${SOURCE_DIR}

TEMP_FILES=*.al *.lc *.tok lines.en lm_eval.test.txt \
           train_en.lm train_en.lm.* \
           out.dynmap out.manual lm_eval.test.mapped
include ../Makefile.incl

txtfiles := $(shell cd ${SOURCE_DIR} && ls hans*.txt)

tokenize: $(txtfiles:%.txt=%.tok)

%_en.tok: %_en.txt
	tokenize.pl -ss -lang=en $< $@
%_fr.tok: %_fr.txt
	tokenize.pl -ss -lang=fr $< $@


align: $(txtfiles:%.txt=%.al)

%_fr.al %_en.al: %_en.tok %_fr.tok
	ssal -f $*_en.tok $*_fr.tok
	mv $*_en.tok.al $*_en.al
	mv $*_fr.tok.al $*_fr.al

lowercase: $(txtfiles:%.txt=%.lc)

%.lc: %.al
	lc-latin.pl $< $@

split: train_en.al train_fr.al

totalsize = $(shell cat $(txtfiles:%.txt=%.lc) | wc -l)
devtestsize = 100
trainsize = $(shell perl -e "print $(totalsize) / 2 - 3 * $(devtestsize)")

train_%.al: $(txtfiles:%.txt=%.lc)
ifndef txtfiles
	@echo No text files to process.
	@echo This directory must contain some parallel text files before you start.
	@exit 1
else
	rm -f lines.$*; touch lines.$*
	echo "$(trainsize)  train_$*.al" >> lines.$*
	echo "$(devtestsize) dev1_$*.al" >> lines.$*
	echo "$(devtestsize) dev2_$*.al" >> lines.$*
	echo "$(devtestsize) test_$*.al" >> lines.$*
	cat *_$*.lc | split-listed.pl lines.$*
endif

lm: train_en.lm
train_en.lm: train_en.al
	ngram-count -interpolate -kndiscount -order 3 -text $< -lm $@

number: train_en.lm.number
train_en.lm.${MAP_TYPE}: train_en.al
	cat $< | map_number -map ${MAP_TYPE} | ngram-count -interpolate -kndiscount -order 3 -text - -lm $@

lm_eval.test.txt:
	echo 1111 > $@
	echo 37 >> $@
	echo 123.978 >> $@
	echo 16 >> $@
	echo bid >> $@
	echo 2nd bid >> $@

dynmap_visual_test: lm_eval.test.txt train_en.lm train_en.lm.${MAP_TYPE}
	lm_eval -v train_en.lm lm_eval.test.txt
	lm_eval -v 'DynMap;${MAP_TYPE};train_en.lm.${MAP_TYPE}' lm_eval.test.txt

dynmap_cmp: lm_eval.test.txt train_en.lm train_en.lm.${MAP_TYPE}
	lm_eval -v 'DynMap;${MAP_TYPE};train_en.lm.${MAP_TYPE}' lm_eval.test.txt | sed 's/.*=//' > out.dynmap
	map_number -map ${MAP_TYPE} < lm_eval.test.txt > lm_eval.test.mapped
	lm_eval -v train_en.lm.${MAP_TYPE} lm_eval.test.mapped | sed 's/.*=//' > out.manual
	diff out.dynmap out.manual -q --ignore-matching-lines="Found k-grams"
