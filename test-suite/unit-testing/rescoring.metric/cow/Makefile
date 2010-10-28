#!/usr/bin/make -f
# vim:noet:ts=3

.NOTPARALLEL:
DATA_DIR ?= ../data

.PHONY: all
all: cow

.PHONY: help
help:
	@echo "Run cow using either bleu/per/wer"
	@cat $(firstword $(MAKEFILE_LIST)) | egrep '^.PHONY:' | sed 's#^.PHONY: ##'


.PHONY: clean
clean:
	-${RM} canoe.ini.cow.*
	-${RM} log.cow.*
	-${RM} rescore-results.*
	-${RM} run-parallel-logs-local

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
	diff ref/rescore-results.$* rescore-results.$* -q
	diff ref/canoe.ini.cow.$* canoe.ini.cow.$* -q
