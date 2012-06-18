#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# $Id$

# @file Makefile
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


MERGE := merge_multi_column_counts

-include Makefile.params

vpath input% src/

.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


.PHONY: clean
clean:
	${RM} unittest1.out unittest2.out

.PHONY: testsuite
testsuite:  unittest1
testsuite:  unittest2


.PHONY: unittest1 unittest2
unittest1 unittest2:  %:  %.out
	diff $< ref/$@ -q

# Merge by tallying the counts.
unittest1.out:  input1 input2 input3
	${MERGE} $@ $+

# Merge using fillup.
unittest2.out:  input1 input2 input3
	${MERGE} -fillup $@ $+



