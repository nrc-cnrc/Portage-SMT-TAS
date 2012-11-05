# This makefile runs the toy example
# vim:ts=3:noet
#
# George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada


all: caveat test

.PHONY: caveat
caveat:
	@echo Running the toy example.
	@echo Warning: do not use this example as a starting point for real experiments.
	@echo Use the framework provided with PORTAGEshared instead.

toy.pdf: toy.tex
	TEXINPUTS=${PORTAGE}/texmf: pdflatex -interaction=batchmode toy.tex
	TEXINPUTS=${PORTAGE}/texmf: pdflatex -interaction=batchmode toy.tex

doc: toy.pdf

.PHONY: clean tokenize align lowercase split lm tm cow rat translate rescore test all

clean:
	rm -f *.aux *.log *.tok *.al *.lc lines.*
	rm -f ibm[12].* phrases.*
	rm -f canoe.ini.* powellweights.* rescore-results* *.FILT log.*
	rm -f multi.probs.dev1_fr.al.30.FILT.gz
	rm -rf foos tmp
	rm -rf workdir-dev2_fr.al-100best workdir-test_fr.al-100best
	rm -f *.tmp* dev*.al.* test*.al.* rat rescore-model
	rm -f *.out *.bleu

txtfiles := $(wildcard *.txt)

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
	train_ibm -v  -bin  -s ibm1.fr_given_en ibm2.fr_given_en $+ >& log.train_ibm.fr_given_en
	train_ibm -vr -bin  -s ibm1.en_given_fr ibm2.en_given_fr $+ >& log.train_ibm.en_given_fr
	gen_phrase_tables -v  -multipr both -w1 -m8 -1 en -2 fr ibm2.fr_given_en ibm2.en_given_fr $+ >& log.gen_phrase_tables

cow: canoe.ini.cow
canoe.ini.cow: phrases.fr2en train_en.lm canoe.ini
	rm -rf foos; mkdir foos
	cow.sh -I-really-mean-cow -nbest-list-size 100 -maxiter 10 -filt -floor 2 \
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
	rat.sh -n 1 trans -v -n 1 -f canoe.ini.cow -K 100 $< test_fr.al
	mv test_fr.al.rat $@

test: rescore_ON_test_fr.bleu canoe_ON_test_fr.bleu
%.bleu: %.out
	bleumain $< test_en.al > $@

bootstrap: rescore_ON_test_fr.out canoe_ON_test_fr.out
	bleucompare $^ REFS test_en.al
