#!/usr/bin/make -f
# vim:noet:ts=3

.NOTPARALLEL:
DATA_DIR ?= ../data


.PHONY: all
all: cow

.PHONY: clean
clean:
	-${RM} canoe.ini.cow.*
	-${RM} log.cow.*
	-${RM} rescore-results.*

.PHONY: cow 
cow: bleu per wer
.PHONY: bleu per wer
bleu: diff.bleu
per: diff.per
wer: diff.wer

vpath dev1_%.al ${DATA_DIR}
cow.%: dev1_fr.al dev1_en.al
	test -d foos.$* || mkdir foos.$*
	cow.sh \
		-$* \
		-floor 2 \
		-nbest-list-size 100 \
		-maxiter 3 \
		-workdir foos.$* \
		$+ \
		&> log.$@
	${RM} -rf foos.$* canoe.ini.FILT.cow canoe.ini.FILT multi.probs.dev1_fr.al.30.FILT.gz
	mv rescore-results rescore-results.$*
	mv canoe.ini.cow canoe.ini.cow.$*

diff.%: cow.%
	diff -q rescore-results.$* ref/rescore-results.$*
	diff -q canoe.ini.cow.$* ref/canoe.ini.cow.$*
