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
	giza_convert_ttable \
	giza_filter \
	map_number \
	markup_canoe_output \
	sample_parallel_text \
	ssal

DYNLIBS= $(LIBICU)

PGM_WITH_ALTERNATIVE_HELP = ssal

MODULE_DEPENDS = tm

BINSCRIPTS = $(wildcard *.pl *.py *.sh)

PERL_LIBS = $(wildcard *.pm)

include ../build/Makefile.incl
