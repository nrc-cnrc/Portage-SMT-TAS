# Makefile for Portage utils

# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

PERL_LIBS= $(wildcard *.pm)

PROGRAMS=compile_truecase_map

BINSCRIPTS= $(wildcard *.pl)

include ../build/Makefile.incl

.PHONY: tests

tests: $(TESTPROGS)
