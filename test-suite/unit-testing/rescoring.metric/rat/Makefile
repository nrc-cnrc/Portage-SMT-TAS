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

.PHONY:  bleu per wer
bleu:  diff.bleu
per:   diff.per
wer:   diff.wer

rt.%:  dev2_fr.al 100best.gz dev2_en.al
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
	! diff rescore-model.$*  ${DATA_DIR}/workdir-dev2_fr.al-100best/rescore-model.ini.rat -q || ! echo "Error: the trained model file should be different than the original model file." >&2
	[[ `wc -l < scores.$*` -gt 1 ]] || ! echo "Error: there should be more than one iteration" >&2
