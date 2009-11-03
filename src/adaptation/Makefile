# Makefile for Portage adapt

# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
	doc_vect.o

LIBRARY = libportage_adapt

PROGRAMS = $(TESTPROGS) \
	cosine_distances \
	mx-dist2weights

BINSCRIPTS = \
        mx-calc-distances.sh \
        mx-mix-models.sh \
	multi-ngram-ppx \
	train-lm-mixture

# Module/directory specific compile flags
MODULE_CF=

include ../build/Makefile.incl
-include Makefile.depend
