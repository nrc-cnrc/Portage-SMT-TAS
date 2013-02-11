# Makefile for models built on tightly packed tries
#
# Author: Ulrich Germann / Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

OBJECTS = \
   tplm.o \
   tppt.o

TESTPROGS =

PROGRAMS = \
   $(TESTPROGS) \
   arpalm.assemble \
   arpalm.encode \
   arpalm.sng-av \
   ptable.assemble \
   ptable.encode-phrases \
   ptable.encode-scores \
   tppt.dump

BINSCRIPTS = \
   arpalm2tplm.sh \
   textldm2tpldm.sh \
   textpt2tppt.sh

LIBRARY = libportage_tp_models

MODULE_CF = -DIN_PORTAGE=1 -ggdb

MODULE_DEPENDS = word_align tpt

DYNLIBS = -lboost_program_options -lboost_system

#DOCUMENTS = doc/TSA.pdf

include ../build/Makefile.incl

