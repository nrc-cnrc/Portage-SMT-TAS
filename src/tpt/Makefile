# Makefile for stuff related to tightly packed tries
#
# Author: Ulrich Germann / Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

OBJECTS = \
   bitwise.o \
   repos_getSequence.o \
   tplm.o \
   tppt.o \
   tpt_encodingschemes.o \
   tpt_error.o \
   tpt_pickler.o \
   tpt_repos.o \
   tpt_tightindex.o \
   tpt_tokenindex.o \
   tpt_utils.o \
   ugReadWriteValues.o \
   ug_ctrack_base.o \
   ug_mm_ctrack.o \
   ug_mm_sufa.o \
   ug_sufa_base.o \
   ug_vocab.o

TESTPROGS =

PROGRAMS = \
   $(TESTPROGS) \
   arpalm.assemble \
   arpalm.encode \
   arpalm.sng-av \
   find_similar_sentences \
   mmctrack.build \
   mmctrack.dump \
   mmsufa.build \
   mmsufa.dump \
   phrasepair-contingency \
   ptable.assemble \
   ptable.encode-phrases \
   ptable.encode-scores \
   tokenindex.dump \
   tppt.dump \
   vocab.build \
   vocab.dump

BINSCRIPTS = \
   arpalm2tplm.sh \
   build-tp-suffix-array.sh \
   textldm2tpldm.sh \
   textpt2tppt.sh \
   vocabMap2tpt.sh

LIBRARY = libportage_tpt

MODULE_CF = -DIN_PORTAGE=1 -ggdb

MODULE_DEPENDS=

EXTRA_LIBS = -lboost_program_options${BOOSTLIBTAG}

DYNLIBS =

include ../build/Makefile.incl
