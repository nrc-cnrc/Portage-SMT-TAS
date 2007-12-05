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
	lmtext.o

PROGRAMS = lmtext2binlm testlm

TESTPROGS = testlm

LIBRARY = libportage_lm

MODULE_CF =

BINSCRIPTS = ngram-count-big.sh

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS =

include ../build/Makefile.incl
-include Makefile.depend
