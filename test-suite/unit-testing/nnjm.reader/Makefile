#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Validates a NNJM model format.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada
# Copyright 2017, Her Majesty in Right of Canada


TEMP_FILES = bin_* txt_*
include ../Makefile.incl

-include Makefile.params
SHELL := bash
TEST_PORTAGE_NNJM := test_portage_nnjm

.DEFAULT_GOAL := all
.PHONY: all
all:  testSuite


.PHONY: clean
clean:



.PHONY:  testSuite
testSuite:

# Disabled since it requires a GPU
#nnjm.bin  nnjm.txt:
#	PYTHONPATH=../../../src/nn python createModels.py


################################################################################
# BINARY MODE
.PHONY: testSuiteBin
testSuite:  testSuiteBin

########################################
# LAYER
testSuiteBin:  bin_layer_w_incomplete
.PHONY: bin_layer_w_incomplete
bin_layer_w_incomplete:  nnjm.bin
	sed '5 s/.\{24\}$$//' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format. Layers's W[5] is incomplete."  --quiet


testSuiteBin:  bin_layer_b_incomplete
.PHONY: bin_layer_b_incomplete
bin_layer_b_incomplete:  nnjm.bin
	sed '5 s/.\{260\}$$//' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layer's b is incomplete."  --quiet


testSuiteBin:  bin_layer_description_short
.PHONY: bin_layer_description_short
bin_layer_description_short:  nnjm.bin
	sed '4 s/.\{2\}$$//' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layers' description doesn't have the right number of parameters."  --quiet


testSuiteBin:  bin_layer_description_long
.PHONY: bin_layer_description_long
bin_layer_description_long:  nnjm.bin
	sed '4 s/$$/ 4/' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layers' description doesn't have the right number of parameters."  --quiet


testSuiteBin:  bin_layer_description_missing
.PHONY: bin_layer_description_missing
bin_layer_description_missing:  nnjm.bin
	sed '4 s/.\{16\}$$//; 5 D' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layer have no descriptor."  --quiet


########################################
# EMBEDDING
testSuiteBin:  bin_embedding_w_incomplete
.PHONY: bin_embedding_w_incomplete
bin_embedding_w_incomplete:  nnjm.bin
	sed '2 s/\[target\].*//;  3,$$ D' nnjm.bin | hexdump -ve '1/1 "%.2x"' | sed '1 s/.\{16\}$$//' | xxd -r -p > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  The embedding[4] is incomplete."  --quiet


testSuiteBin:  bin_embedding_description_short
.PHONY: bin_embedding_description_short
bin_embedding_description_short:  nnjm.bin
	sed '1 s/.\{2\}$$//' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings' descriptor format doesn't have the right number of parameters."  --quiet


testSuiteBin:  bin_embedding_description_long
.PHONY: bin_embedding_description_long
bin_embedding_description_long:  nnjm.bin
	sed '1 s/$$/ 4/' $< > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings' descriptor format doesn't have the right number of parameters."  --quiet


testSuiteBin:  bin_embedding_description_missing
.PHONY: bin_embedding_description_missing
bin_embedding_description_missing:  nnjm.bin
	cat /dev/null > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings are missing their descriptor."  --quiet


testSuiteBin:  bin_empty_model
.PHONY: bin_empty_model
bin_empty_model:
	cat /dev/null > $@.bin
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.bin  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings are missing their descriptor."  --quiet



################################################################################
# TEXT MODE
.PHONY: testSuiteTxt
testSuite:  testSuiteTxt

########################################
# LAYER
testSuiteTxt:  txt_layer_w_incomplete
.PHONY: txt_layer_w_incomplete
txt_layer_w_incomplete:  nnjm.txt
	sed '$$ s/\( [^ ]\+\)\{2\}$$//' $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format. Layers's W[5] is incomplete."  --quiet


testSuiteTxt:  txt_layer_w_partial
.PHONY: txt_layer_w_partial
txt_layer_w_partial:  nnjm.txt
	head -n 26 $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  The layer has no data for its W[2]."  --quiet


testSuiteTxt:  txt_layer_b_incomplete
.PHONY: txt_layer_b_incomplete
txt_layer_b_incomplete:  nnjm.txt
	< $< head -n 24 | sed '$$ s/\( [^ ]\+\)\{2\}//' > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layer's b is incomplete."  --quiet


testSuiteTxt:  txt_layer_b_partial
.PHONY: txt_layer_b_partial
txt_layer_b_partial:  nnjm.txt
	head -n 23 < $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  The layer has no data for its b."  --quiet


testSuiteTxt:  txt_layer_description_short
.PHONY: txt_layer_description_short
txt_layer_description_short:  nnjm.txt
	sed '23 s/.\{2\}$$//' $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layers' description doesn't have the right number of parameters."  --quiet


testSuiteTxt:  txt_layer_description_long
.PHONY: txt_layer_description_long
txt_layer_description_long:  nnjm.txt
	sed '23 s/$$/ 4/' $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layers' description doesn't have the right number of parameters."  --quiet


testSuiteTxt:  txt_layer_description_missing
.PHONY: txt_layer_description_missing
txt_layer_description_missing:  nnjm.txt
	head -n 22 $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Layer have no descriptor."  --quiet


########################################
# EMBEDDING
testSuiteTxt:  txt_embedding_w_incomplete
.PHONY: txt_embedding_w_incomplete
txt_embedding_w_incomplete:  nnjm.txt
	sed '6 s/\( [^ ]\+\)\{2\}$$//' < $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embedding[4] is incomplete."  --quiet


testSuiteTxt:  txt_embedding_w_partial
.PHONY: txt_embedding_w_partial
txt_embedding_w_partial:  nnjm.txt
	head -n 5 < $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  The embeddings have not enough data."  --quiet


testSuiteTxt:  txt_embedding_description_short
.PHONY: txt_embedding_description_short
txt_embedding_description_short:  nnjm.txt
	sed '1 s/.\{2\}$$//' $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings' descriptor format doesn't have the right number of parameters."  --quiet


testSuiteTxt:  txt_embedding_description_long
.PHONY: txt_embedding_description_long
txt_embedding_description_long:  nnjm.txt
	sed '1 s/$$/ 4/' $< > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings' descriptor format doesn't have the right number of parameters."  --quiet


testSuiteTxt:  txt_embedding_description_missing
.PHONY: txt_embedding_description_missing
txt_embedding_description_missing:  nnjm.txt
	cat /dev/null > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings are missing their descriptor."  --quiet


testSuiteTxt:  txt_empty_model
.PHONY: txt_empty_model
txt_empty_model:
	cat /dev/null > $@.txt
	${TEST_PORTAGE_NNJM}  -native -s 2 -n 2  $@.txt  <(echo '1 1 / 1 / 4') \
	|& grep --fixed-strings "Error: Invalid model format.  Embeddings are missing their descriptor."  --quiet
