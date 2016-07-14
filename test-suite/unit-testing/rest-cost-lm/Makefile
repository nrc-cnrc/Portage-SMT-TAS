# @file Makefile
# @brief Test suite for rest-cost LMs
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

TEMP_FILES=log.* train.wc LM.rclm.synopsis prob.* lm-train_fr.al.gz
TEMP_DIRS=LM.rclm LM.rclm.tmp.*
include ../Makefile.incl

SHELL:=/bin/bash

all: check_input check_LM.rclm test_probs

RAW_TRAIN_CORPUS=${PORTAGE}/test-suite/tutorial-data/lm-train_fr.raw.gz
#TRAIN_CORPUS=${PORTAGE}/test-suite/tutorial-data/lm-train_fr.al.gz
TRAIN_CORPUS=lm-train_fr.al.gz

${TRAIN_CORPUS}: ${RAW_TRAIN_CORPUS}
	utokenize.pl -lang=fr -noss $< $@

# Make sure the training data did not change on us...
check_input: ${TRAIN_CORPUS}
	gzip -cqdf ${TRAIN_CORPUS} | wc_stats > train.wc
	diff train.wc ref/train.wc

LM.rclm/config: ${TRAIN_CORPUS}
	train-rest-cost-lm.sh -text ${TRAIN_CORPUS} -lm LM.rclm >& log.LM.rclm

# This short extract from the LM was examined manually and is reasonable
LM.rclm.synopsis: LM.rclm/config
	zcat LM.rclm/5g.lm.gz | egrep 'ngram|officiel\b' > $@
	zcat LM.rclm/rest-costs.lm.gz | egrep 'ngram|officiel\b' >> $@

check_LM.rclm: LM.rclm.synopsis
	diff-round.pl -p 2 $< ref/$<
	diff LM.rclm/config ref/config

# Calculate probabilities using RCLM in TPLM and ARPA format, and using regular 5g LM
probs: prob.5g prob.rclm prob.rclm.arpa

prob.5g: queries LM.rclm/config
	lm_eval -1qpl LM.rclm/5g.lm.tplm $< > $@

prob.rclm: queries LM.rclm/config
	lm_eval -1qpl LM.rclm $< > $@

prob.rclm.arpa: queries LM.rclm/config config.arpalm
	lm_eval -1qpl config.arpalm $< > $@

# Check the resuls of the LM queries done above.
test_probs: probs cmp_prob.5g cmp_prob.rclm cmp_prob.rclm.arpa relative_check_probs

cmp_prob.%: prob.%
	diff-round.pl -p 3 $< ref/$<
cmp_prob.rclm.arpa: prob.rclm.arpa
	diff-round.pl -p 3 $< ref/prob.rclm

relative_check_probs: probs
	diff-round.pl -p 5 prob.rclm prob.rclm.arpa
	[[ `diff-round.pl prob.5g prob.rclm | wc -l` == 5 ]]

