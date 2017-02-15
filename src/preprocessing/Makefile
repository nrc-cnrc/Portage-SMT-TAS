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
	split_parallel_text_by_tag \
	ssal

DYNLIBS= $(LIBICU)

PGM_WITH_ALTERNATIVE_HELP = ssal

MODULE_DEPENDS = word_align

BINSCRIPTS = $(wildcard *.pl *.py *.sh)

PERL_LIBS = $(wildcard *.pm)

include ../build/Makefile.incl


test:  check_python_dependencies

.PHONY:  check_python_dependencies
check_python_dependencies:
	echo MADAMIRA_HOME: $$MADAMIRA_HOME
	test ! -d "$$MADAMIRA_HOME" || python -c 'import xmltodict'
