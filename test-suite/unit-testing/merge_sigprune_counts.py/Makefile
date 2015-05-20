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


$(info This unittest has been reused in order to test the merge_sigprune_counts implementation in C++)
SHELL := bash
MERGE := merge_sigprune_counts

-include Makefile.params


vpath jpt% src
vpath %.ref ref

.PHONY: all
all: testsuite


TEMP_FILES=unittest2.gz
include ../Makefile.incl


.PHONY: testsuite
testsuite: unittest0
testsuite: unittest1
testsuite: unittest2
testsuite: unittest3
testsuite: unittest4
testsuite: unittest5


# Check error if no argument is provide to merge_sigprune_counts.py.
# merge_sigprune_counts.py takes at least two argument.
.PHONY: unittest0
unittest0: 
	${MERGE} 2>&1 | grep 'Error: too few arguments' --quiet
	${MERGE} $+ 2>&1 | grep 'Error: too few arguments' --quiet



# Check noop if there is only one file.
# Merging only one shard must produce a result that must be identical to its input.
.PHONY: unittest1
unittest1: jpt00000 jpt00000_mmsufa0.out
	${MERGE} $+ | diff - <(paste $(word 2, $+) $(word 1, $+)) -q



# Check proper merge of sevaral shards.
.PHONY: unittest2
unittest2: unittest2.gz  unittest2.ref
	zcmp $+

unittest2.gz: jpt00000 jpt00000_mmsufa0.out jpt00000_mmsufa1.out jpt00000_mmsufa2.out
	${MERGE} $+ | gzip > $@



# Detect one shard that is too short.
# All shards must be of same length.
.PHONY: unittest3
unittest3: jpt00000 jpt00000_mmsufa0.out jpt00000_mmsufa1.out jpt00000_mmsufa2.out
	${MERGE} $< $(word 2, $+) <(head $(word 3, $+)) $(word 4, $+) 2>&1 | egrep "Error: .* is too short" --quiet



# Detect shards that are not aligned.
# shards< counts should be of same length of jpt.
.PHONY: unittest4
unittest4: jpt00000 jpt00000_mmsufa0.out jpt00000_mmsufa1.out jpt00000_mmsufa2.out
	${MERGE} <(head $<) $(wordlist 2,4,$+)  2>&1 | egrep 'Error: .* is too long!' --quiet



# Validate the jpt's format.
.PHONY: unittest5
unittest5: jpt00000_mmsufa0.out jpt00000_mmsufa1.out jpt00000_mmsufa2.out
	${MERGE} $+ 2>&1 | egrep 'Error: .* is not a joint phrase table' --quiet

