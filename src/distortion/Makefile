# Makefile for distortion models.
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
	dmstruct.o

LIBRARY = libportage_distortion

PROGRAMS = \
	dmcount \
	dmestm

DOCUMENTS = 

BINSCRIPTS = filter-distortion-model.pl

DYNLIBS = $(LIBICU)

MODULE_CF= 

MODULE_DEPENDS = tm

include ../build/Makefile.incl
-include Makefile.depend
