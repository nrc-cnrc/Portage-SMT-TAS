#!/usr/bin/make -f
# vim:ts=3:noet

# Makefile - unit test fix-en-fr-numbers.pl and other number handling scripts
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

all: test

test: test-utf8 test-latin1 test-en test-fr

test-%: out.%
	diff $< ref/$<

#test-utf8: out.utf8
#	diff $< ref/$<
#
test-latin1: out.latin1 ref/out.latin1
	diff $< ref/$<

in.latin1 ref/out.latin1: %.latin1: %.utf8
	iconv -f utf-8 -t iso-8859-1 < $< > $@

out.%: in.%
	fix-en-fr-numbers.pl < $< > $@

TEMP_FILES=out.* in.latin1 ref/out.latin1
include ../Makefile.incl
gitignore:
	echo ref/out.latin1 >> .gitignore

out.en: in.en
	utokenize.pl -noss < $< | mark-numbers-en2fr.pl > $@

out.fr: in.fr
	utokenize.pl -noss -lang=fr < $< | mark-numbers-fr2en.pl > $@
