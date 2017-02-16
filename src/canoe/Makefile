# @file Makefile
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

INCLUDES=$(filter-out canoe_help.h filter_models.h, \
           $(wildcard *.h))

OBJECTS= \
    alignedphrase.o \
    alignment_annotation.o \
    backwardsmodel.o \
    basicmodel.o \
    bilm_annotation.o \
    bilm_model.o \
    canoe_general.o \
    canoe_constants.o \
    config_io.o \
    count_annotation.o \
    annotation_list.o \
    cube_pruning_decoder.o \
    cube_pruning_hyp_stack.o \
    decoder.o \
    decoder_feature.o \
    decoderstate.o \
    distortionmodel.o \
    forced_phrase_finder.o \
    hard_filter_tm_visitor.o \
    hypothesisstack.o \
    ibm_feature.o \
    inputparser.o \
    levenshtein_feature.o \
    marked_translation.o \
    mixtm_feature.o \
    multiprob_pt_feature.o \
    nbesttranslation.o \
    new_src_sent_info.o \
    ngrammatch_feature.o \
    nnjm.o \
    partialtranslation.o \
    phrasefinder.o \
    phrasetable.o \
    phrasetable_feature.o \
    phrasetable_filter_grep.o \
    phrasetable_filter_joint.o \
    phrasetable_filter_lm.o \
    pruning_style.o \
    rule_feature.o \
    segmentmodel.o \
    simple_overlay.o \
    shift_reducer.o \
    soft_filter_tm_visitor.o \
    sparsemodel.o \
    tppt_feature.o \
    translationProb.o \
    unal_feature.o \
    walls_zones.o \
    wordgraph.o

LIBRARY=libportage_canoe

PERL_LIBS=

TESTPROGS= \
	testconfig

IGNORES=tests/test_*.MMmap tests/test_*.txt

PROGRAMS=$(TESTPROGS) \
	canoe \
	configtool \
	count_multi_prob_columns \
	filter_models \
	join_phrasetables \
	mix_phrasetables \
	palminer \
	palview \
	palstats

BINSCRIPTS= \
	build-sparse-model.sh \
	canoe-parallel.sh \
	canoe-timing-stats.pl \
	nbest2rescore.pl \
	translate.pl

# config_io.o is very expensive to optimize, for some reason.
config_io.o: OPT_LEVEL=-O0

DOCUMENTS=canoe.pdf

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=$(LIBICU) -lboost_system -lboost_filesystem

canoe: DYNLIBS += -lpthread

MODULE_CF=

MODULE_DEPENDS=logging tm word_align lm tp_models tpt preprocessing nn

include ../build/Makefile.incl

canoe.dvi: uml.eps canoe.bib

uml.eps: uml.fig
	fig2dev -L eps $^ $@

all:  translate.pl.svg
translate.pl.svg:  translate.dot
	-@dot -Tsvg < $< > $@

