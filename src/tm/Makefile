# Makefile for translation models.
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
	ibm.o \
	phrase_table.o \
	phrase_table_reader.o \
	tm_io.o \
	ttable.o \
	ttablewithmax.o \
	word_align.o

LIBRARY = \
	libportage_tm

PROGRAMS = \
	align-words \
	bag_trans \
	gen_phrase_tables \
	ibm_entropy \
	joint2cond_phrase_tables \
	merge_ttables \
	phrase_align_stats \
	phrase_oracle \
	run_ibm \
	tester \
	train_ibm \
	try \
	tt2pt \
	tt_pt_merge

TESTPROGS = tester try

BINSCRIPTS = train-phrases.sh maxphrase.pl tmtext_sort.sh gen-jpt-parallel.sh

DOCUMENTS =

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS = -lpthread

MODULE_CF = -Wno-non-virtual-dtor

MODULE_DEPENDS = logging

include ../build/Makefile.incl
-include Makefile.depend
