# Makefile for Portage utils

# @author George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

INCLUDES = $(filter-out Prog.h, $(wildcard *.h))

PROGRAMS =

OBJECTS = \
   argProcessor.o \
   blanklayout.o \
   logging.o \
   LogLevel.o

LIBRARY=libportage_logging

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=

MODULE_CF=

include ../build/Makefile.incl
-include Makefile.depend
