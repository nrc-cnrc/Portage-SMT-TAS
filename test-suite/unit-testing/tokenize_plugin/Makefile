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

TOKENIZE_PLUGIN = tokenize_plugin

-include Makefile.params

.DEFAULT_GOAL := all
.PHONY: all
all:  compounds.ar
all:  compounds.ar.bw.mada.tok.parse_tokan


TEMP_FILES = compounds.ar.txt  compounds.ar.bw.mada.tok.parse_tokan.txt

include ../Makefile.incl


# Check that compounded glyphs are not separated
.PHONY:  compounds.ar
compounds.ar:  compounds.ar.txt  ref/compounds.ar
	diff $+ --brief

# This is acutally code taken from tokenize_plugin but since running MADA is slow, this gives us a quicker test.
compounds.ar.txt:  src/compounds.ar
	perl -ple 'BEGIN{use encoding "UTF-8"; } s/(\p{Script=Arabic}\p{General_Category:Mark}*)([^\p{Script=Arabic}\p{General_Category:Mark}])/$$1 $$2/g; s/([^\p{Script=Arabic}\p{General_Category:Mark}])(\p{Script=Arabic}\p{General_Category:Mark}*)/$$1 $$2/g;' < $< > $@


.PHONY:  compounds.ar.bw.mada.tok.parse_tokan
compounds.ar.bw.mada.tok.parse_tokan:  compounds.ar.bw.mada.tok.parse_tokan.txt  ref/compounds.ar.bw.mada.tok.parse_tokan
	diff $+ --brief

compounds.ar.bw.mada.tok.parse_tokan.txt:  src/compounds.ar
	${TOKENIZE_PLUGIN} ar < $< > $@
