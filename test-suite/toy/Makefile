# This makefile runs the toy example
#
# George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada


all: test

toy.pdf: toy.tex
	pdflatex -interaction=batchmode toy.tex
	pdflatex -interaction=batchmode toy.tex

doc: toy.pdf

.PHONY: clean tokenize align lowercase split lm tm cow rat translate rescore test all

clean:
	rm -f *.aux *.log *.tok *.al *.lc lines.*
	rm -f ibm[12].* phrases.*
	rm -f canoe.ini.* powellweights.* rescore-results* *.FILT log.*
	rm -rf foos tmp
	rm -f *.tmp* dev*.al.* test*.al.* rat rescore-model
	rm -f *.out *.bleu

txtfiles := $(wildcard *.txt)

tokenize: $(txtfiles:%.txt=%.tok)

%_en.tok: %_en.txt
	tokenize.pl -lang=en $< $@
%_fr.tok: %_fr.txt
	tokenize.pl -lang=fr $< $@


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
	@if which-test.sh ngram-count; then \
	    echo ngram-count -interpolate -kndiscount -order 3 -text $< -lm $@ ; \
	    ngram-count -interpolate -kndiscount -order 3 -text $< -lm $@ ; \
	elif [ -f train_en.lm ]; then \
	    echo Warning: ngram-count not found, keeping existing $@ file ; \
	else \
	    echo Error: ngram-count not found, can\'t make $@ ; exit 1 ; \
	fi

tm: phrases.fr2en
phrases.fr2en: train_en.al train_fr.al
	rm -rf tmp; mkdir tmp
	ln -s ../train_en.al tmp/train_en.al
	ln -s ../train_fr.al tmp/train_fr.al
	train-phrases.sh -no-z tmp

cow: canoe.ini.cow
canoe.ini.cow: phrases.fr2en train_en.lm
	rm -rf foos; mkdir foos
	cow.sh -nbest-list-size 100 -maxiter 10 -filt -floor 2 \
		-workdir foos dev1_fr.al dev1_en.al &> log.cow

rat: rescore-model
rescore-model: canoe.ini.cow
	rat.sh -n 1 train -n 1 -f $< -K 100 -o $@ \
		rescore-model.ini dev2_fr.al dev2_en.al &> log.rat

translate: canoe_ON_test_fr.out
canoe_ON_test_fr.out: canoe.ini.cow
	canoe -f $< < test_fr.al > $@

rescore: rescore_ON_test_fr.out
rescore_ON_test_fr.out: rescore-model
	rat.sh -n 1 trans -n 1 -f canoe.ini.cow -K 100 $< test_fr.al
	mv test_fr.al.rat $@

test: rescore_ON_test_fr.bleu canoe_ON_test_fr.bleu
%.bleu: %.out
	bleumain $< test_en.al > $@

bootstrap: rescore_ON_test_fr.out canoe_ON_test_fr.out
	bleucompare $^ REFS test_en.al
