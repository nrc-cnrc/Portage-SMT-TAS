# Makefile for language models
#
# Bruno Laferriere
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
	lm.o \
        lmbin.o \
        lmbin_vocfilt.o \
        lmmix.o \
	lmtext.o

PROGRAMS = lmtext2binlm lm_eval

LIBRARY = libportage_lm

MODULE_CF =

BINSCRIPTS = lm_sort_filter.sh ngram-count-big.sh

EXTRA_LIBS =

DYNLIBS =

include ../build/Makefile.incl
-include Makefile.depend
