#!/usr/bin/make -Bf
# vim:noet:ts=3

SCOREMAIN ?= bleumain
DATA_DIR  ?= ../data
BLEUMAIN := bleumain

vpath rescore_ON_test_fr.out ${DATA_DIR}
vpath test_en%.al ${DATA_DIR}
vpath weights% ${DATA_DIR}

.PHONY: all
all: bleu wer per
all:  weighted


.PHONY: wer
wer:; ${MAKE} scoremain.wer SCOREMAIN=wermain

.PHONY: per
per:; ${MAKE} scoremain.per SCOREMAIN="wermain -per"

.PHONY: bleu
bleu:; ${MAKE} scoremain.bleu SCOREMAIN=bleumain

.PHONY: scoremain
scoremain.%: rescore_ON_test_fr.out test_en1.al test_en2.al test_en3.al
	${SCOREMAIN} -c -detail 2 $+ | diff-round.pl - ref/$*main -q


.PHONY:  weighted
weighted:  bleu.weights
weighted:  bleu.weights_all_ones

.PHONY:  bleu.weights bleu.weights_all_ones
bleu.weights  bleu.weights_all_ones:  bleu.%:  %  rescore_ON_test_fr.out test_en1.al test_en2.al test_en3.al
	${BLEUMAIN} -c -detail 2 -w $+ | diff-round.pl - ref/bleumain.$* -q

