# Makefile for translation models.
#
# George Foster
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

SOURCES = \
	align-words.cc \
	bag_trans.cc \
	gen_phrase_tables.cc \
	ibm.cc \
	ibm_entropy.cc \
	joint2cond_phrase_tables.cc \
	merge_ttables.cc \
	phrase_align_stats.cc \
	phrase_oracle.cc \
	phrase_table.cc \
	phrase_table_reader.cc \
	run_ibm.cc \
	tester.cc \
	tm_io.cc \
	train_ibm.cc \
	tt2pt.cc \
	tt_pt_merge.cc \
	ttable.cc \
	ttablewithmax.cc \
	word_align.cc

INCLUDES = \
	ibm.h \
	length.h \
	phrase_smoother.h \
	phrase_smoother_cc.h \
	phrase_table.h \
	phrase_table_reader.h \
	tm_io.h \
	ttable.h \
	ttablewithmax.h \
	word_align.h

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

BINSCRIPTS = train-phrases.sh maxphrase.pl tmtext_sort.sh

DOCUMENTS =

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS = -lpthread

MODULE_CF = -Wno-non-virtual-dtor

MODULE_DEPENDS = logging

DOXYFILE = ../build/Doxyfile

include ../build/Makefile.incl
-include Makefile.depend
