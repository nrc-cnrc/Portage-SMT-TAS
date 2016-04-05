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


TEMP_FILES=canoe.ini.cow.* log.cow.* rescore-results* run-parallel-logs-*
TEMP_DIRS=foos.* run-p.*
include ../../Makefile.incl

.PHONY: cow
cow: bleu per wer
.PHONY: bleu per wer
bleu: diff.bleu
per: diff.per
wer: diff.wer

# Since tune.py doesn't support wer and per yet, we still have to use cow.sh here.
vpath dev1_%.al ${DATA_DIR}
cow.%: dev1_fr.al dev1_en.al
	test -d foos.$* || mkdir foos.$*
	cow.sh -v -I-really-mean-cow \
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
	diff ref/rescore-results.$* rescore-results.$* -q -s || diff ref/rescore-results.$*-alt rescore-results.$* -q -s || diff ref/rescore-results.$*-alt2 rescore-results.$* -q -s
	diff ref/canoe.ini.cow.$* canoe.ini.cow.$* -q -s || diff ref/canoe.ini.cow.$*-alt canoe.ini.cow.$* -q -s
