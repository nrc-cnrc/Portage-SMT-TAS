#!/usr/bin/make -f
# vim:noet:ts=3


BLEUCOMPARE ?= bleucompare
DATA_DIR    ?= ../data

vpath %.out ${DATA_DIR}
vpath test_en%.al ${DATA_DIR}

.PHONY: all
all: bleu per wer

.PHONY: bleu
bleu: compare.bleu
.PHONY: per
per:; ${MAKE} compare.per BLEUCOMPARE="${BLEUCOMPARE} -per"
.PHONY: wer
wer:; ${MAKE} compare.wer BLEUCOMPARE="${BLEUCOMPARE} -wer"


compare.%: canoe_ON_test_fr.out rescore_ON_test_fr.out  test_en1.al test_en2.al test_en3.al
	${BLEUCOMPARE} $(wordlist 1,2,$+) REFS $(wordlist 3,5,$+) | diff -q - ref/bleucompare.$*
