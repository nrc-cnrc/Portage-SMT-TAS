# $Id$# @file Makefile
# @file Makefile
# 
# Evaluation Module
# Aaron Tikuisis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology 
# Conseil national de recherches Canada / National Research Council Canada 
# Copyright 2004, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, Her Majesty in Right of Canada 

INCLUDES=$(filter-out bestbleu.h bleumain.h wermain.h, $(wildcard *.h))

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
ifdef PORTAGE_OPENMP
MODULE_CF=$(PORTAGE_OPENMP)
MODULE_LF=$(PORTAGE_OPENMP)
#else
MODULE_CF=-Wno-unknown-pragmas
endif

MODULE_DEPENDS=logging

LIBRARY=libportage_eval

include ../build/Makefile.incl
-include Makefile.depend
