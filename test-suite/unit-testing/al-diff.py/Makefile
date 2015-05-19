#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Testsuite for al-diff.py.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


-include Makefile.params

AL_DIFF ?= al-diff.py

vpath %alignment.1 data
vpath %alignment.2 data
vpath %source data
vpath %target data

SHELL = bash
.PHONY: all
all: testsuite


.PHONY: clean
clean:



.PHONY: testsuite
testsuite: $(addprefix unittest., A B C D MULTIDOC)
testsuite: unittest.EMPTYTAG
testsuite: unittest1
testsuite: unittest.MULTIDOC.diff



.PHONY: unittest.A unittest.B unittest.C unittest.ALL
unittest.MULTIDOC: MARKER=-m "<PORTAGE_DOCUMENT_END>"
$(addprefix unittest., A B C D MULTIDOC): unittest.%: %.alignment.1 %.alignment.2 %.source %.target
	${AL_DIFF} ${MARKER} $+ | diff -q - ref/$@

# Let's try a multi docs where the document marker is an empty line.
unittest.EMPTYTAG: MULTIDOC.alignment.1 MULTIDOC.alignment.2
	${AL_DIFF} -m "" <(sed 's/<PORTAGE_DOCUMENT_END>//' $(filter %1, $+)) <(sed 's/<PORTAGE_DOCUMENT_END>//' $(filter %2, $+)) | diff -q - ref/$@

# We MUST provide source and target when using -v
unittest1: A.alignment.1 A.alignment.2
	! ${AL_DIFF} -v $+ 2> /dev/null

unittest.MULTIDOC.diff: $(addprefix MULTIDOC, .alignment.1 .alignment.2 .source .target)
	${AL_DIFF} -v -m "<PORTAGE_DOCUMENT_END>" $+ | diff -q - ref/$@



################################################################################
# HELPERS

# Checking the claim that al-diff.py works with multidocs separated by an empty line.
debug:
	${AL_DIFF} -m "" \
		<(sed -e 's/<PORTAGE_DOCUMENT_END>//' data/MULTIDOC.alignment.1) \
		<(sed -e 's/<PORTAGE_DOCUMENT_END>//' data/MULTIDOC.alignment.2) \
		<(sed -e 's/<PORTAGE_DOCUMENT_END>//' data/MULTIDOC.source) \
		<(sed -e 's/<PORTAGE_DOCUMENT_END>//' data/MULTIDOC.target)

