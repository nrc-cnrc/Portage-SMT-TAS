# $Id$# @file Makefile
# 
# Evaluation Module
# Aaron Tikuisis
# 
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
# Institut de technologie de l'information / Institute for Information Technology 
# Conseil national de recherches Canada / National Research Council Canada 
# Copyright 2004, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, Her Majesty in Right of Canada 

SOURCES = \
	basic_data_structure.cc \
	bestbleu.cc \
	bestbleucompute.cc \
	bestwer.cc \
	bleu.cc \
	bleucompare.cc \
	bleumain.cc \
	referencesReader.cc \
	wer.cc \
	wermain.cc

INCLUDES = \
	basic_data_structure.h \
	bestbleu.h \
	bleu.h \
	fileReader.h  \
	referencesReader.h \
	wer.h

OBJECTS = \
	basic_data_structure.o \
	bestbleucompute.o \
	bleu.o \
	referencesReader.o \
	wer.o 

PROGRAMS = \
	bestbleu \
	bestwer \
	bleucompare \
	bleumain \
	wermain

BINSCRIPTS = \
	CalculateHypothesisProb.pl

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=-lpthread

# Module/directory specific compile flags
MODULE_CF= -Wno-non-virtual-dtor

MODULE_DEPENDS=logging

LIBRARY=libportage_eval

DOXYFILE = ../build/Doxyfile

include ../build/Makefile.incl

-include Makefile.depend
