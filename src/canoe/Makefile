# $Id$# @file Makefile
# 
# Canoe Decoder
# Aaron Tikuisis
# 
# Groupe de technologies langagières interactives / Interactive Language Technologies Group 
# Institut de technologie de l'information / Institute for Information Technology 
# Conseil national de recherches Canada / National Research Council Canada 
# Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada 
# 
# This is the Makefile for this module.

# Note: you need the Portage dependency libraries to be available in the library
# and include search paths.  These paths should be given by Makefile.incl.  As a
# sidenote, if you want to specify these paths via environment variables, type
# the following (in bash):
#  export LIBRARY_PATH=$LIBRARY_PATH:/export/projets/portage/lib
#  export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/export/projets/portage/include

SOURCES= \
    alignedphrase.cc \
    backwardsmodel.cc \
    basicmodel.cc \
    canoe.cc \
    canoe_general.cc \
    canoe_constants.cc \
    config_io.cc \
    configtool.cc \
    decoder.cc \
    decoder_feature.cc \
    decoderstate.cc \
    distortionmodel.cc \
    filter_models.cc \
    find_sentence_phrases.cc \
    forced_phrase_finder.cc \
    forced_target_model.cc \
    forced_target_phrasetable.cc \
    hypothesisstack.cc \
    inputparser.cc \
    partialtranslation.cc \
    phrasefinder.cc \
    phrasetable.cc \
    phrase_tm_align.cc \
    phrase_tm_align_compute.cc \
    segmentmodel.cc \
    testconfig.cc \
    testdecoder.cc \
    testhypstack.cc \
    testphrasefinder.cc \
    translationProb.cc \
    wordgraph.cc \
    nbesttranslation.cc

INCLUDES= \
    alignedphrase.h \
    backwardsmodel.h \
    basicmodel.h \
    decoder.h \
    decoder_feature.h \
    canoe_general.h \
    canoe_help.h \
    config_io.h \
    distortionmodel.h \
    filter_models.h \
    forced_phrase_finder.h \
    forced_target_model.h \
    forced_target_phrasetable.h \
    inputparser.h \
    hypothesisstack.h \
    length_feature.h \
    nbesttranslation.h \
    phrasedecoder_model.h \
    phrasefinder.h \
    phrasetable.h \
    phrase_tm_align.h \
    segmentmodel.h \
    wordgraph.h

OBJECTS= \
    alignedphrase.o \
    backwardsmodel.o \
    basicmodel.o \
    canoe_general.o \
    canoe_constants.o \
    config_io.o \
    decoder.o \
    decoder_feature.o \
    decoderstate.o \
    distortionmodel.o \
    forced_phrase_finder.o \
    forced_target_model.o \
    forced_target_phrasetable.o \
    hypothesisstack.o \
    inputparser.o \
    nbesttranslation.o \
    partialtranslation.o \
    phrasefinder.o \
    phrasetable.o \
    phrase_tm_align_compute.o \
    segmentmodel.o \
    translationProb.o \
    wordgraph.o

LIBRARY=libportage_canoe

PERL_LIBS=

TESTPROGS=testhypstack testphrasefinder testdecoder testconfig

PROGRAMS=$(TESTPROGS) canoe configtool \
        filter_models \
	find_sentence_phrases \
	phrase_tm_align

BINSCRIPTS=nbest2rescore.pl canoe-parallel.sh 

INFOFILES=

DOCUMENTS=canoe.pdf

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=

DOXYFILE=../build/Doxyfile

# Module/directory specific compile flags
MODULE_CF=-Wno-non-virtual-dtor

MODULE_DEPENDS=logging tm lm

include ../build/Makefile.incl

canoe.dvi: uml.eps canoe.bib

uml.eps: uml.fig
	fig2dev -L eps $^ $@

# Override rules for programs:
ifndef LIBRARY
testdecoder:
	$(CXX) $(CXXFLAGS) -o $@ testdecoder.o decoder.o decoderstate.o \
	    partialtranslation.o hypothesisstack.o phrasefinder.o \
	    canoe_constants.o $(LINKWITH) $(DYNLIBS)

testhypstack:
	$(CXX) $(CXXFLAGS) -o $@ testhypstack.o hypothesisstack.o partialtranslation.o \
	    decoderstate.o canoe_constants.o $(LINKWITH) $(DYNLIBS)

testphrasefinder:
	$(CXX) $(CXXFLAGS) -o $@ testphrasefinder.o phrasefinder.o partialtranslation.o \
	    canoe_constants.o $(LIBS) $(LINKWITH) $(DYNLIBS)
endif

-include Makefile.depend
