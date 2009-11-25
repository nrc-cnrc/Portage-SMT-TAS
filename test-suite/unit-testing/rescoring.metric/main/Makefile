#!/usr/bin/make -Bf
# vim:noet:ts=3

SCOREMAIN ?= bleumain
DATA_DIR  ?= ../data

vpath rescore_ON_test_fr.out ${DATA_DIR}
vpath test_en%.al ${DATA_DIR}

.PHONY: all
all: bleu wer per


.PHONY: wer
wer:; ${MAKE} scoremain.wer SCOREMAIN=wermain

.PHONY: per
per:; ${MAKE} scoremain.per SCOREMAIN="wermain -per"

.PHONY: bleu
bleu:; ${MAKE} scoremain.bleu SCOREMAIN=bleumain

.PHONY: scoremain
scoremain.%: rescore_ON_test_fr.out test_en1.al test_en2.al test_en3.al
	${SCOREMAIN} -c -detail 2 $+ | diff - ref/$*main -q
