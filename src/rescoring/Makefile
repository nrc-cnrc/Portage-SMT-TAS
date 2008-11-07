# $Id$

# K-Best Rescoring Module
# Aaron Tikuisis
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l.information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

INCLUDES = $(filter-out rescore_train.h rescore_translate.h \
                        rescore_test.h, $(wildcard *.h))

OBJECTS = \
	bleu_postedit_ff.o \
        backward_lm_ff.o \
	bleurisk.o \
	cache_lm_ff.o \
	confidence_score.o \
	consensus.o \
	featurefunction_set.o \
        file_ff.o \
	fmax.o \
	ibm1wtrans.o \
	ibm1del.o \
	ibm_ff.o \
	lm_ff.o \
	mismatch_ff.o \
	multiColumnFileFF.o \
	nbest_ngrampost.o \
	nbest_phrasepost_src.o \
	nbest_phrasepost_trg.o \
	nbest_posterior.o \
	nbest_posterior_ff.o \
	nbest_sentlenpost.o \
	nbest_wordpost_lev.o \
	nbest_wordpost_src.o \
	nbest_wordpost_trg.o \
	ngram_tree.o \
	per_postedit_ff.o \
	rescore_io.o \
	wer_postedit_ff.o

BINSCRIPTS = \
	cow.sh \
	gen-features-parallel.pl \
	rat.sh \
	rescoremodel2canoearg.sh

LIBRARY=libportage_rescoring

TESTPROGS = \
	test_feature_function_grammar \
	test_rescoring_stop \
	testff \
	testfileReader \
	testfmax \
	testlinemax \
	testpowell

PROGRAMS=$(TESTPROGS) \
        feature_function_tool \
	gen_feature_values \
	rescore_test \
	rescore_train \
	rescore_translate

DOCUMENTS=rescoring_overview.pdf ochlinemax.pdf

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=-lpthread -licuuc

ifdef NO_PORTAGE_OPENMP
MODULE_CF=-Wno-non-virtual-dtor -fno-strict-aliasing -Wno-uninitialized
else
MODULE_CF=-Wno-non-virtual-dtor -fno-strict-aliasing -Wno-uninitialized -fopenmp
MODULE_LF=-fopenmp
endif

MODULE_DEPENDS=canoe tm lm eval logging

include ../build/Makefile.incl

rescoring_overview.dvi: rescoring_overview.bib

.PHONY: tests

tests: $(TESTPROGS)

-include Makefile.depend
