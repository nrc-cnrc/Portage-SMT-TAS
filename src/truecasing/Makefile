# Makefile for Portage truecasing
# $Id$

# George Foster; updated by Darlene Stewart
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, 2011 Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, 2011 Her Majesty in Right of Canada

OBJECTS = pylm.o

LIBRARY = libportage_truecasing

PERL_LIBS=

PROGRAMS = \
	compile_truecase_map \
	normc1

BINSCRIPTS= \
	truecase.pl \
	boscap.py \
	boscap-nosrc.py \
	casemark.py \
	filter-nc1.py

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS = $(LIBICU)

MODULE_DEPENDS = lm logging tpt

MODULE_CF =

include ../build/Makefile.incl

all: dyn_lib
install: install_dyn_lib
