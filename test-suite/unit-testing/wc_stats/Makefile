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
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada



WC_STATS := wc_stats

-include Makefile.params


.PHONY: all
all:  testsuite


TEMP_FILES=a.gz b.gz c.gz unittest1.gz
include ../Makefile.incl


.PHONY: testsuite
testsuite:  unittest1



# In BOOST prior to 1.44, there was a problem where the BOOST iostream lib
# wouldn't properly read an archive created by concatenating more than 3 gzip
# files.
.INTERMEDIATE: a.gz b.gz c.gz unittest1.gz
.PHONY: unittest1
unittest1: unittest1.gz
	@[[ `${WC_STATS} -m $+ | grep TOTAL | grep '^L:3'` ]] || ! echo "The total should indicate that there are 3 lines total." >&2
	@[[ `${WC_STATS} -m $+ | grep -v TOTAL | grep -c '^L:3'` == 1 ]] || ! echo "There should be one file with exactly 3 lines!" >&2

unittest1.gz: a.gz b.gz c.gz
	cat $+ > $@






################################################################################
# HELPERS
%.gz:
	echo $* | gzip > $@
