#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada


-include Makefile.params

.DEFAULT_GOAL := all
.PHONY: all
all:  compounds.ar


TEMP_FILES = compounds.ar.txt

include ../Makefile.incl


# Check that compounded glyphs are not separated
compounds.ar:  compounds.ar.txt  ref/compounds.ar
	diff $+ --brief

compounds.ar.txt:  src/compounds.ar
	perl -ple 'BEGIN{use encoding "UTF-8"; } s/(\p{Script=Arabic}\p{General_Category:Mark}*+)(\P{Script=Arabic})/$$1 $$2/g; s/(\P{Script=Arabic})(\p{Script=Arabic}\p{General_Category:Mark}*+)/$$1 $$2/g;' < $< > $@

