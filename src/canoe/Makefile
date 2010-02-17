# $Id$# @file Makefile
#
# Canoe Decoder
# Aaron Tikuisis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, Her Majesty in Right of Canada
#
# This is the Makefile for this module.

# Note: you need the Portage dependency libraries to be available in the
# library and include search paths.  These paths should be given by
# Makefile.incl.  As a sidenote, if you want to specify these paths via
# environment variables, type the following (in bash):
#  export LIBRARY_PATH=$LIBRARY_PATH:/export/projets/portage/lib
#  export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/export/projets/portage/include

INCLUDES=$(filter-out canoe_help.h filter_models.h phrase_tm_align.h, \
           $(wildcard *.h))

OBJECTS= \
    alignedphrase.o \
    backwardsmodel.o \
    basicmodel.o \
    canoe_general.o \
    canoe_constants.o \
    config_io.o \
    cube_pruning_decoder.o \
    cube_pruning_hyp_stack.o \
    decoder.o \
    decoder_feature.o \
    decoderstate.o \
    distortionmodel.o \
    forced_phrase_finder.o \
    forced_target_phrasetable.o \
    hard_filter_tm_visitor.o \
    hypothesisstack.o \
    ibm_feature.o \
    inputparser.o \
    levenshtein_feature.o \
    marked_translation.o \
    nbesttranslation.o \
    ngrammatch_feature.o \
    partialtranslation.o \
    phrasefinder.o \
    phrasetable.o \
    phrasetable_filter_grep.o \
    phrasetable_filter_joint.o \
    phrasetable_filter_lm.o \
    phrase_tm_align_compute.o \
    pruning_style.o \
    rule_feature.o \
    segmentmodel.o \
    soft_filter_tm_visitor.o \
    translationProb.o \
    wordgraph.o

LIBRARY=libportage_canoe

PERL_LIBS=

TESTPROGS= \
	testconfig

PROGRAMS=$(TESTPROGS) \
	canoe \
	configtool \
	count_multi_prob_columns \
	filter_models \
	join_phrasetables \
	mix_phrasetables \
	phrase_tm_align

BINSCRIPTS=nbest2rescore.pl canoe-parallel.sh

DOCUMENTS=canoe.pdf

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=$(LIBICU)

MODULE_CF=

MODULE_DEPENDS=logging tm lm tpt

include ../build/Makefile.incl

canoe.dvi: uml.eps canoe.bib

uml.eps: uml.fig
	fig2dev -L eps $^ $@
