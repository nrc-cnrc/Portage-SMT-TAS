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
	lmtext.o \
	lmtrie.o

PROGRAMS = \
	arpalm2binlm \
	binlm2arpalm \
	lm_eval \
	test_lm_order \
	train_lm_mixture

BINSCRIPTS = \
	lm_sort.pl \
	lm_sort_filter.sh \
	ngram-count-big.sh \
	lm-order.pl

LIBRARY = libportage_lm

MODULE_CF =

MODULE_DEPENDS = tpt

EXTRA_LIBS = $(TPT_LIBS)

DYNLIBS = $(LIBICU)

include ../build/Makefile.incl
