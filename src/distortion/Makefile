# Makefile for distortion models.
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS := \
	distortion_algorithm.o \
	dmstruct.o

LIBRARY := libportage_distortion

PROGRAMS := \
	dmcount \
	dmestm

DOCUMENTS := 

BINSCRIPTS := filter-distortion-model.pl

DYNLIBS = $(LIBICU)

# Module/directory specific compile flags
MODULE_CF :=

MODULE_DEPENDS = tm word_align logging

EXTRA_LIBS = $(LOGGING_LIB)

include ../build/Makefile.incl
