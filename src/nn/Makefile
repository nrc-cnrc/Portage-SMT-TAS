# @file Makefile
# @brief Makefile for Portage Neural Net (NNJM).
# 
# @author George Foster and Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, 2017 Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, 2017 Her Majesty in Right of Canada

OBJECTS = nnjm_native.o nnjm_abstract.o

LIBRARY = libportage_nn

PYTHON_LIBS = \
	embed.pyc \
	msgd.pyc \
	multilayers.pyc \
	nnjm_data_iterator.py \
	nnjm_utils.pyc

TESTPROGS = test_portage_nnjm

PROGRAMS = $(TESTPROGS)

BINSCRIPTS = \
	train-nnjm.sh \
	nnjm-genex.py \
	train-nnjm.py \
	unpickle.py

include ../build/Makefile.incl
