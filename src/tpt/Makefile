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
   tpt_encodingschemes.o \
   tpt_error.o \
   tpt_pickler.o \
   tpt_repos.o \
   tpt_tightindex.o \
   tpt_tokenindex.o \
   tpt_utils.o \
   ugReadWriteValues.o \
   ug_corpus_token.o \
   ug_ctrack_base.o \
   ug_get_options.o \
   ug_mm_ctrack.o \
   ug_mm_sufa.o \
   ug_sufa_base.o \
   ug_tsa_array_entry.o \
   ug_ttrack_base.o \
   ug_ttrack_position.o \
   ug_vocab.o

TESTPROGS =

PROGRAMS = \
   $(TESTPROGS) \
   find_similar_sentences \
   mmctrack.build \
   mmctrack.dump \
   mmsufa.build \
   mmsufa.dump \
   phrasepair-contingency \
   tokenindex.dump \
   vocab.build \
   vocab.dump \
   mmtsa.find

BINSCRIPTS = \
   build-tp-suffix-array.sh \
   vocabMap2tpt.sh

LIBRARY = libportage_tpt

MODULE_CF = -DIN_PORTAGE=1 -ggdb

MODULE_DEPENDS =

DYNLIBS = -lboost_program_options -lboost_system

install: clean_moved_programs
.PHONY: clean_moved_programs
clean_moved_programs:
	@${RM} \
	arpalm.assemble \
	arpalm.encode \
	arpalm.sng-av \
	ptable.assemble \
	ptable.encode-phrases \
	ptable.encode-scores \
	tppt.dump

include ../build/Makefile.incl
