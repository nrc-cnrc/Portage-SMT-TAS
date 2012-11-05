# This makefile runs the toy example
# vim:ts=3:noet
#
# George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada


all: caveat test

.PHONY: caveat
caveat:
	@echo Running a toy test suite for unit testing purposes.
	@echo Warning: do not use this example as a starting point for real experiments.
	@echo Use the framework provided with PORTAGEshared instead.

toy.pdf: toy.tex
	pdflatex -interaction=batchmode toy.tex
	pdflatex -interaction=batchmode toy.tex

doc: toy.pdf

.PHONY: clean tokenize align lowercase split tm cow rat translate rescore test all

clean:
	rm -f *.aux *.log *.tok *.al *.lc lines.*
	rm -f ibm[12].* phrases.*
	rm -f canoe.ini.* powellweights.* rescore-results* *.FILT log.*
	rm -f multi.probs.dev1_fr.al.30.FILT.gz
	rm -rf foos tmp
	rm -rf workdir-dev2_fr.al-100best workdir-test_fr.al-100best
	rm -f *.tmp* dev*.al.* test*.al.* rat rescore-model
	rm -f *.out *.bleu

txtfiles := $(wildcard *.txt.gz)

tokenize: $(txtfiles:%.txt.gz=%.tok)

%_en.tok: %_en.txt.gz
	zcat < $< | tokenize.pl -lang=en - $@
%_fr.tok: %_fr.txt.gz
	zcat < $< | tokenize.pl -lang=fr - $@


align: $(txtfiles:%.txt.gz=%.al)

%_fr.al %_en.al: %_en.tok %_fr.tok
	ssal -f $*_en.tok $*_fr.tok
	mv $*_en.tok.al $*_en.al
	mv $*_fr.tok.al $*_fr.al

lowercase: $(txtfiles:%.txt.gz=%.lc)

%.lc: %.al
	lc-latin.pl $< $@

split: train_en.al train_fr.al

totalsize = $(shell cat $(txtfiles:%.txt.gz=%.lc) | wc -l)
devtestsize = 100
trainsize = $(shell perl -e "print $(totalsize) / 2 - 3 * $(devtestsize)")

train_%.al: $(txtfiles:%.txt.gz=%.lc)
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

lm: train_en.lm.gz

tm: phrases.fr2en
phrases.fr2en: train_en.al train_fr.al
	rm -rf tmp; mkdir tmp
	ln -s ../train_en.al tmp/train_en.al
	ln -s ../train_fr.al tmp/train_fr.al
	train-phrases.sh -no-z tmp

canoe.ini: phrases.fr2en train_en.lm.gz
	@echo "[ttable-multi-prob] phrases.fr2en" > $@
	@echo "[lmodel-file] train_en.lm.gz" >> $@
	@echo "[ttable-limit] 30" >> $@
	@echo "[ttable-threshold] 0.000" >> $@
	@echo "[stack] 70" >> $@
	@echo "[beam-threshold] 0.01" >> $@
	@echo "[distortion-limit] 7" >> $@

cow: canoe.ini.cow
canoe.ini.cow: canoe.ini
	@echo "Training decoder weights"
	rm -rf foos; mkdir foos
	cow.sh -I-really-mean-cow -nbest-list-size 100 -maxiter 10 -filt -floor 2 \
		-workdir foos dev1_fr.al dev1_en.al &> log.cow

rescore-model.ini: canoe.ini.cow 
	configtool rescore-model:ffvals < $< | cut -f 1 -d' ' > $@
	echo "SCRIPT:\"rescore-bwd-lm.sh <src> <nbest> train_en.lm.gz\"" >> $@
	echo "IBM1DeletionSrcGivenTgt:ibm1.fr_given_en#0.2" >> $@
	echo "IBM1DeletionTgtGivenSrc:ibm1.en_given_fr#0.2" >> $@
	echo "IBM1SrcGivenTgt:ibm1.fr_given_en" >> $@
	echo "IBM1TgtGivenSrc:ibm1.en_given_fr" >> $@
	echo "IBM1WTransSrcGivenTgt:ibm2.fr_given_en" >> $@
	echo "IBM1WTransTgtGivenSrc:ibm2.en_given_fr" >> $@
	echo "IBM2SrcGivenTgt:ibm2.fr_given_en" >> $@
	echo "IBM2TgtGivenSrc:ibm2.en_given_fr" >> $@
	echo "LengthFF" >> $@
	echo "ParMismatch" >> $@
	echo "QuotMismatch:fe" >> $@
	echo "nbestWordPostLev:1#<ffval-wts>#<pfx>" >> $@
	echo "nbestWordPostTrg:1#<ffval-wts>#<pfx>" >> $@
	echo "nbestNgramPost:3#1#<ffval-wts>#<pfx>" >> $@
	echo "nbestSentLenPost:1#<ffval-wts>#<pfx>" >> $@
	echo "nbestWordPostSrc:1#<ffval-wts>#<pfx>" >> $@
	echo "nbestPhrasePostSrc:1#<ffval-wts>#<pfx>" >> $@
	echo "nbestPhrasePostTrg:1#<ffval-wts>#<pfx>" >> $@


rat: rescore-model
rescore-model: canoe.ini.cow rescore-model.ini
	@echo "Training a rescoring model"
	rat.sh -n 2 train -dep -v -d -n 2 -f $< -K 100 -o $@ \
		rescore-model.ini dev2_fr.al dev2_en.al &> log.rat

translate: canoe_ON_test_fr.out
canoe_ON_test_fr.out: canoe.ini.cow
	canoe -f $< < test_fr.al > $@

rescore: rescore_ON_test_fr.out
rescore_ON_test_fr.out: rescore-model
	@echo "Rescoring test set"
	rat.sh -n 2 trans -dep -v -n 2 -f canoe.ini.cow -K 100 $< test_fr.al &> log.rescored.test
	mv test_fr.al.rat $@

test: rescore_ON_test_fr.bleu canoe_ON_test_fr.bleu
%.bleu: %.out
	bleumain $< test_en.al > $@

bootstrap: rescore_ON_test_fr.out canoe_ON_test_fr.out
	bleucompare $^ REFS test_en.al
