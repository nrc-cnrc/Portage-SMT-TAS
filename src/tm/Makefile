# Makefile for translation models.
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
        phrase_pair_extractor.o \
        phrase_table.o \
        phrase_table_reader.o

LIBRARY = libportage_tm

TESTPROGS = \
        try

PROGRAMS = $(TESTPROGS) \
        bag_trans \
        count_phrasefreqs \
        gen_phrase_tables \
        gen_jpt_filter_tool \
        joint2cond_phrase_tables \
        joint2multi_cpt \
        merge_sigprune_counts \
        phrase_align_stats \
        phrase_oracle \
        sigprune_fet \
        sigprune_line_numbering \
        train_tm_mixture \
        tt2pt \
        tt_pt_merge

BINSCRIPTS = \
        add-indicator-to-multiprob-pt.pl \
        fixed_term2tm.pl \
        gen-jpt-parallel.sh \
        sigprune.sh \
        tmtext-apply-weights.pl \
        tmtext_sort.sh \
        tt2ptbest.pl

DOCUMENTS =

EXTRA_LIBS = $(LOGGING_LIB)

DYNLIBS = $(LIBICU)

MODULE_CF = -DHAVE_CXX_STDHEADERS

MODULE_DEPENDS = word_align logging

PGM_WITH_ALTERNATIVE_HELP = \
        extract_phrase_patterns \
        gen_phrase_tables \
        joint2cond_phrase_tables \
        joint2multi_cpt \
        phrase_oracle

include ../build/Makefile.incl

install: clean_old_programs

.PHONY: clean_old_programs
clean_old_programs:
	@${RM} \
	align-words \
	eval_word_alignment \
	ibm_entropy \
	ibmcat \
	run_ibm \
	train_ibm \
	word_align_tool

