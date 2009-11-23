#!/usr/bin/make -f
# vim:noet:ts=3
# Makefile - Unit-test for filter-distortion-model.pl.
# Tests
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

FILTER_DISTORTION_MODEL = filter-distortion-model.pl -d

.INTERMEDIATE:

all: test
all: test.1
all: test.2
all: test.3
all: test.4
all: test.5

clean:
	${RM} cpt.* dm.* filt.*

########################################
# Compare a run to a reference
test: cpt dm filt
	${FILTER_DISTORTION_MODEL} cpt dm \
	| diff -q - filt

test.%: cpt.% dm.% filt.%
	${FILTER_DISTORTION_MODEL} cpt.$* dm.$* \
	| diff -q - filt.$*


########################################
# Create the reference.
filt: cpt dm
	cat cpt \
	| perl -ple 's/\|\|\| [^\|]+$$//' \
	| grep -F -f - dm \
	> $@

filt.%: cpt.% dm.%
	-cat cpt.$* \
	| perl -ple 's/\|\|\| [^\|]+$$//' \
	| grep -F -f - dm.$* \
	> $@


########################################
# test.1
# longer tail for dm.
cpt.1: cpt
	head -n 5 $< > $@

dm.1: dm
	cat $< > $@


########################################
# test.2
# No match
cpt.2: cpt
	cat $< > $@

dm.2: dm
	tail -n 1 $< > $@


########################################
# test.3
# No match
cpt.3: cpt
	tail -n 1 $< > $@

dm.3: dm
	cat $< > $@


########################################
# test.4
# dm starts before cpt.
cpt.4: cpt
	egrep -v '^a' $< > $@

dm.4: dm
	cat $< > $@


########################################
# test.5
# cpt starts before dm.
cpt.5: cpt
	cat $< > $@

dm.5: dm
	egrep -v '^[aA]' $< > $@


