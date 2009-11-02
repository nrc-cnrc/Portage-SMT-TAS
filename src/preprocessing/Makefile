# Makefile for preprocessing programs
#
# @author George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
	docid.o \
	parse_pal.o

LIBRARY = libportage_preprocessing

PROGRAMS = \
	asciify \
        map_number \
        sample_parallel_text \
	ssal

BINSCRIPTS = $(wildcard *.pl)

PERL_LIBS = $(wildcard *.pm)

include ../build/Makefile.incl

-include Makefile.depend
