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
        phrase_pair_extractor.o \
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
        count_phrasefreqs \
        eval_word_alignment \
        gen_phrase_tables \
        gen_jpt_filter_tool \
        ibm_entropy \
        ibmcat \
        joint2cond_phrase_tables \
        joint2multi_cpt \
        merge_sigprune_counts \
        phrase_align_stats \
        phrase_oracle \
        run_ibm \
        sigprune_fet \
        sigprune_line_numbering \
        train_ibm \
        tt2pt \
        tt_pt_merge \
        word_align_tool

BINSCRIPTS = \
        add-indicator-to-multiprob-pt.pl \
        cat.sh \
        gen-jpt-parallel.sh \
        sigprune.sh \
        tmtext-apply-weights.pl \
        tmtext_sort.sh \
        tt2ptbest.pl

DOCUMENTS =

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS = $(LIBICU)

MODULE_CF = -DHAVE_CXX_STDHEADERS

MODULE_DEPENDS = logging

PGM_WITH_ALTERNATIVE_HELP = \
        align-words \
        extract_phrase_patterns \
        gen_phrase_tables \
        joint2cond_phrase_tables \
        joint2multi_cpt \
        phrase_oracle

include ../build/Makefile.incl
