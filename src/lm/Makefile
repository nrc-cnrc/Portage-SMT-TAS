# Makefile for language models
#
# Bruno Laferriere
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

SOURCES = \
	lm.cc \
        lmbin.cc \
	lmtext.cc \
        lmtext2binlm.cc \
	testlm.cc

INCLUDES = \
	lm.h \
        lmbin.h \
	lmtext.h

OBJECTS = \
	lm.o \
        lmbin.o \
	lmtext.o

PROGRAMS = lmtext2binlm testlm

TESTPROGS = testlm

LIBRARY = libportage_lm

DOXYFILE = ../build/Doxyfile

MODULE_CF =

BINSCRIPTS = ngram-count-big.sh

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS =

include ../build/Makefile.incl
-include Makefile.depend
