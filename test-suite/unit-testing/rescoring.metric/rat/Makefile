#!/usr/bin/make -f
# vim:ts=3:noet

RESCORE_TRAIN ?= rescore_train
DATA_DIR      ?= ../data

vpath dev2_%.al ${DATA_DIR}
vpath 100best.gz ${DATA_DIR}/workdir-dev2_fr.al-100best

.PHONY: all
all: bleu per wer

TEMP_FILES=scores.* rescore-model.* log.*
include ../../Makefile.incl

.PHONY: bleu per wer
bleu: diff.bleu
per: diff.per
wer: diff.wer

rt.%: dev2_fr.al 100best.gz dev2_en.al
	-${RM} scores.$*
	${RESCORE_TRAIN} \
		-$* \
		-v -n \
		-wo scores.$* \
		-p ${DATA_DIR}/workdir-dev2_fr.al-100best/ \
		${DATA_DIR}/workdir-dev2_fr.al-100best/rescore-model.ini.rat \
		rescore-model.$* \
		$+ \
		>& log.$*

diff.%: rt.%
	diff rescore-model.$* ref/rescore-model.$* -q
	diff-round.pl -sort -p 5 scores.$* ref/scores.$* -q
