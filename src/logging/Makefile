# Makefile for Portage utils

# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

SOURCES = $(filter-out Prog.cc, $(wildcard *.cc))

INCLUDES = $(filter-out Prog.h, $(wildcard *.h))

OBJECTS = \
   argProcessor.o \
   blanklayout.o \
   logging.o \
   LogLevel.o

LIBRARY=libportage_logging

PROGRAMS= 

TESTPROGS=  

BINSCRIPTS= 

INFOFILES=

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=-lpthread

DOCUMENTS= 

# Module/directory specific compile flags
MODULE_CF=-Wno-non-virtual-dtor

include ../build/Makefile.incl
-include Makefile.depend
