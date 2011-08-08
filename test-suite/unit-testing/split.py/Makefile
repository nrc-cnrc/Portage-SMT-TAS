#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# $Id$

# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


SPLIT := split.py

-include Makefile.params


SHELL = bash

.PHONY: all
all: testsuite


.PHONY: clean
clean:
	${RM} input.gz
	${RM} unittest*


.PHONY: testsuite
testsuite: unittest1
testsuite: unittest2
testsuite: unittest3
testsuite: unittest4
testsuite: unittest5
testsuite: unittest6
testsuite: unittest7



########################################
# Extract all even lines.
.PHONY: unittest1
unittest1:
	${SPLIT} -i 0 -m 2 <(seq 0 10) | diff -q - <(seq 0 2 10)


########################################
# Extract all odd lines.
.PHONY: unittest2
unittest2:
	${SPLIT} -i 1 -m 2 <(seq 0 10) | diff -q - <(seq 1 2 10)


########################################
# Make sure that we are not losing any lines when extracting even lines and extracting odd lines.
.PHONY: unittest3
unittest3:
	[[ `cat <(${SPLIT} -i 0 -m 2 <(seq 0 10)) <(${SPLIT} -i 1 -m 2 <(seq 0 10)) | \wc -l` == 11 ]] || ! echo "We are missing some entries!" >&2


########################################
# Validate the index based on the modulo.
.PHONY: unittest4
unittest4:
	${SPLIT} -i 2 -m 2 <(seq 0 10) 2>&1 | grep -qF 'Error the index is 2 but must be in the range [0, 2)'


########################################
# Extract all odd lines from standard input.
.PHONY: unittest5
unittest5:
	seq 0 10 | ${SPLIT} -i 1 -m 2 | diff -q - <(seq 1 2 10)


########################################
# Extract all odd lines from compressed file.
.PHONY: unittest6
unittest6:  input.gz
	${SPLIT} -i 1 -m 2 $+ | diff -q - <(seq 1 2 10)


########################################
# Rebuild from stipe output.
.PHONY: unittest7  unittest7p unittest7l
unittest7: unittest7p
unittest7: unittest7l

# Rebuild from a pattern.
unittest7p: $(addprefix unittest7., 0 1 2)
	${SPLIT} -r unittest7. | diff -q - <(seq 0 20)

# Rebuild from a list of files.
unittest7l: $(addprefix unittest7., 0 1 2)
	${SPLIT} -r $+ | diff -q - <(seq 0 20)

unittest7.%:
	${SPLIT} -i $* -m 3 <(seq 0 20) $@



################################################################################
# HELPERS
input.gz:
	seq 0 10 | gzip > $@
