# $Id$
# K-Best Rescoring Module
# Aaron Tikuisis
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l.information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

SOURCES = \
   featurefunction.cpp \
   fmax.cc \
   gen_feature_values.cc \
   ibm1aaron.cc \
   ibm_ff.cc \
   ibm1del.cc \
   linemax.cpp \
   multiColumnFileFF.cc \
   rescore_io.cc \
   rescore_test.cc \
   rescore_train.cc \
   rescore_translate.cc \
   lm_ff.cpp \
   testff.cpp \
   testfileReader.cc \
   testfmax.cc \
   testlinemax.cpp \
   testpowell.cpp

INCLUDES = \
   featurefunction.h \
   fmax.h \
   rescore_io.h \
   rescoring_general.h \
   ibm1aaron.h \
   ibm_ff.h \
   ibm1del.h \
   linemax.h \
   powell.h \
   lm_ff.h \
   featurefunction.h

OBJECTS = \
   featurefunction.o \
   fmax.o \
   ibm1aaron.o \
   ibm_ff.o \
   ibm1del.o \
   multiColumnFileFF.o \
   rescore_io.o \
   lm_ff.o


LIBRARY=libportage_rescoring

PERL_LIBS=

TESTPROGS=testlinemax testpowell testff testfmax testfileReader 

PROGRAMS=$(TESTPROGS) gen_feature_values rescore_test rescore_train rescore_translate

BINSCRIPTS=cow.sh gen-features-parallel.sh rat.sh rescoremodel2canoearg.sh

INFOFILES=

DOCUMENTS=rescoring_overview.pdf ochlinemax.pdf

EXTRA_LIBS=$(LOGGING_LIB)

DYNLIBS=-lpthread

DOXYFILE = ../build/Doxyfile

MODULE_CF=-Wno-non-virtual-dtor

MODULE_DEPENDS=canoe tm lm eval logging

include ../build/Makefile.incl

rescoring_overview.dvi: rescoring_overview.bib

TARFILES_sfu_local = rescore_train rescore_translate
TARFILES_sfu_ext = ../eval/bleumain ../eval/bestbleu

tarball_sfu: $(TARFILES_sfu_local)
	install -d -m0750 PortageRescoring/bin
	install -ps -m0750 $(TARFILES_sfu_local) $(TARFILES_sfu_ext) PortageRescoring/bin
	ln -fs ../../../test-suite/rescoring-example/README ../../../test-suite/rescoring-example/example ../../../test-suite/rescoring-example/standard PortageRescoring
	tar --exclude CVS -czhf PortageRescoring.tgz PortageRescoring
	rm -r PortageRescoring

.PHONY: tests

tests: $(TESTPROGS)

-include Makefile.depend
