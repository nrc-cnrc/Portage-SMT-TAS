# Makefile for language models
#
# Bruno Laferriere
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
	lm.o \
	lmbin.o \
	lmbin_vocfilt.o \
	lmbin_novocfilt.o \
	lmdynmap.o \
	lmmix.o \
	lmrestcost.o \
	lmtext.o \
	lmtrie.o

PROGRAMS = \
	arpalm2binlm \
	binlm2arpalm \
	lm_eval \
	test_lm_order \
	train_lm_mixture

BINSCRIPTS = \
	gen-word-given-wcl-lm.py \
	lm-add-heuristic-unk.pl \
	lm-filter.py \
	lm-order.pl \
	lm_sort.pl \
	lm_sort_filter.sh \
	ngram-count-big.sh \
	sri-mix-lms.py \
	train-rest-cost-lm.sh

LIBRARY = libportage_lm

MODULE_CF =

MODULE_DEPENDS = tp_models tpt

DYNLIBS = $(LIBICU) -lboost_system

IGNORES=tests/test_*.MMmap tests/test_*.txt

include ../build/Makefile.incl
