# vim:noet:ts=3:nowrap:filetype=make
# Makefile for word-alignment models.
#
# Eric Joanis
# Technologies langagieres interactives / Interactive Language Technologies
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

OBJECTS = \
        alignment_file.o \
        hmm_aligner.o \
        hmm_jump_basic_data.o \
        hmm_jump_classes.o \
        hmm_jump_end_dist.o \
        hmm_jump_map.o \
        hmm_jump_simple.o \
        hmm_jump_strategy.o \
        ibm.o \
        tp_alignment.o \
        ttable.o \
        ttablewithmax.o \
        word_align.o \
        word_align_io.o

LIBRARY = libportage_word_align

PROGRAMS = $(TESTPROGS) \
        align-words \
        eval_word_alignment \
        ibm_entropy \
        ibmcat \
        merge_ttables \
        run_ibm \
        tp_alignment_build \
        tp_alignment_dump \
        train_ibm \
        word_align_tool

BINSCRIPTS = \
        cat.sh

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS = $(LIBICU)

MODULE_CF = -DHAVE_CXX_STDHEADERS

MODULE_DEPENDS = tpt logging

PGM_WITH_ALTERNATIVE_HELP = \
        align-words

include ../build/Makefile.incl

# Additional dependency:
run_tests/test_alignment_file: tp_alignment_build
