# $Id$
# K-Best Rescoring Module
# Aaron Tikuisis
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l.information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

INCLUDES = $(filter-out rescore_train.h rescore_translate.h \
                        rescore_test.h, $(wildcard *.h))

OBJECTS = \
	featurefunction.o \
	fmax.o \
	ibm1wtrans.o \
	ibm1del.o \
	ibm_ff.o \
	lm_ff.o \
	multiColumnFileFF.o \
	rescore_io.o

BINSCRIPTS = \
	cow.sh \
	gen-features-parallel.sh \
	rat.sh \
	rescoremodel2canoearg.sh

LIBRARY=libportage_rescoring

PERL_LIBS=

TESTPROGS = \
	testff \
	testfileReader \
	testfmax \
	testlinemax \
	testpowell

PROGRAMS=$(TESTPROGS) \
	gen_feature_values \
	rescore_test \
	rescore_train \
	rescore_translate

INFOFILES=

DOCUMENTS=rescoring_overview.pdf ochlinemax.pdf

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=-lpthread

MODULE_CF=-Wno-non-virtual-dtor -Wno-uninitialized

MODULE_DEPENDS=canoe tm lm eval logging

include ../build/Makefile.incl

rescoring_overview.dvi: rescoring_overview.bib

.PHONY: tests

tests: $(TESTPROGS)

-include Makefile.depend
