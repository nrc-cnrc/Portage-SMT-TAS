# Makefile for translation models.
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
        hmm_aligner.o \
        hmm_jump_basic_data.o \
        hmm_jump_strategy.o \
        hmm_jump_simple.o \
        hmm_jump_end_dist.o \
        hmm_jump_classes.o \
        hmm_jump_map.o \
	ibm.o \
	phrase_table.o \
	phrase_table_reader.o \
	tm_io.o \
	ttable.o \
	ttablewithmax.o \
	word_align.o \
        word_align_io.o

LIBRARY = libportage_tm

TESTPROGS = \
        try

PROGRAMS = $(TESTPROGS) \
	align-words \
	bag_trans \
        eval_word_alignment \
	gen_phrase_tables \
	ibm_entropy \
	joint2cond_phrase_tables \
	phrase_align_stats \
	phrase_oracle \
	run_ibm \
	train_ibm \
	tt2pt \
	tt_pt_merge \
        word_align_tool


BINSCRIPTS = \
        cat.sh \
        gen-jpt-parallel.sh \
        tmtext_sort.sh \
        train-phrases.sh

DOCUMENTS =

EXTRA_LIBS = $(LOGGING_LIB)

MODULE_CF = -DHAVE_CXX_STDHEADERS

MODULE_DEPENDS = logging

include ../build/Makefile.incl
-include Makefile.depend
