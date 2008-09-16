#!/usr/bin/make -f
# vim:ts=3:noet

RESCORE_TRAIN ?= rescore_train
DATA_DIR      ?= ../data

vpath dev2_%.al ${DATA_DIR}
vpath 100best.gz ${DATA_DIR}/workdir-dev2_fr.al-100best

.PHONY: all
all: bleu per wer

.PHONY: clean
clean:
	-${RM} scores.* rescore-model.*

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
		$+

diff.%: rt.%
	diff -q rescore-model.$* ref/rescore-model.$*
	diff -q scores.$* ref/scores.$*
