#!/usr/bin/make -f
# vim:ts=3:noet:nowrap:filetype=make

# @file Makefile
# @brief Test suite for ssal-pipeline.sh
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2020, Sa Majeste la Reine du Chef du Canada /
# Copyright 2020, Her Majesty in Right of Canada

TEMP_DIRS=models ssal-pipeline.tmp.*
TEMP_FILES=small_*.al
include ../Makefile.incl

all: setup align_small

models:
	mkdir -p $@

models/ibm2.%: ../joint2cond/data/ibm2.% models
	ln -sf ../$< $@

setup: \
   models/ibm2.train.en_given_fr.gz \
   models/ibm2.train.en_given_fr.pos.gz \
   models/ibm2.train.fr_given_en.gz \
   models/ibm2.train.fr_given_en.pos.gz

align_small: ssal_config input/small_en.txt input/small_fr.txt setup
	ssal-pipeline.sh -debug -c $< input/small_en.txt input/small_fr.txt small_en.al small_fr.al
	diff small_en.al ref/ -q
	diff small_fr.al ref/ -q
