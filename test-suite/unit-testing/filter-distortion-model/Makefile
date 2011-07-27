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

SHELL = bash

FILTER_DISTORTION_MODEL := filter-distortion-model.pl -d

.SUFFIXES:
.SECONDARY:

vpath dm% src
vpath cpt% src
vpath filt% ref

all: test.0
all: test.1
all: test.2
all: test.3
all: test.4
all: test.5
all: test.6
all: test.7.gz
all: test-b

clean:
	${RM} cpt* dm* filt* fdm* log*


########################################
# Compare a run to a reference
test%:  fdm% filt%
	[[ `zcat -f $(filter-out %.bkoff, $(filter fdm%, $+)) | wc -l` == ${EXPECTTED_VALUE} ]] || ! echo "Invalid number of entries for $@ expecting ${EXPECTTED_VALUE}" >&2
	[[ -s "$(subst .gz,,$(filter fdm%, $+)).bkoff" ]] || ! echo "You are missing your filtered bkoff file." >&2
	diff -q <(zcat -f $(filter fdm%, $+)) <(zcat -f $(filter filt%, $+))


########################################
# Create filtered distortion models.
fdm%: cpt% dm%
	${FILTER_DISTORTION_MODEL} $+ $@ 2> log$*


########################################
# Create the reference.
REGEX := | perl -ple 's/ \|\|\| /\t/g' | cut -f1,2 | perl -ple 's/(\t|$$)/ ||| /g'
filt.%.gz:  cpt.%.gz dm.%.gz
	{ zcat -f $(filter dm%, $+) | grep -f <(zcat -f $(filter cpt%, $+) ${REGEX}) | LC_ALL=C sort | gzip > $@; } || true

# NOTE: fdm 2 & 3 produce an empty intersection but we don't want an error message thus we add the or true.
filt%:  cpt% dm%
	{ zcat -f $(word 2, $+) | grep -f <(zcat -f $(word 1, $+) ${REGEX}) | LC_ALL=C sort > $@; } || true


########################################
# Create the bkoff file.
dm%.bkoff: dm.bkoff
	ln -fs $< $@


########################################
# fdm-b
test-b: EXPECTTED_VALUE := 6


########################################
# fdm
test.0: EXPECTTED_VALUE := 3
dm.0: dm.0.bkoff
cpt.0: cpt
	ln -fs $< $@

dm.0: dm
	ln -fs $< $@


########################################
# fdm.1
# longer tail for dm.
test.1: EXPECTTED_VALUE := 1
dm.1: dm.1.bkoff
cpt.1: cpt
	head -n 5 $< > $@

dm.1: dm
	ln -fs $< $@


########################################
# fdm.2
# No match
test.2: EXPECTTED_VALUE := 0
dm.2: dm.2.bkoff
cpt.2: cpt
	ln -fs $< $@

dm.2: dm
	tail -n 1 $< > $@


########################################
# fdm.3
# No match
test.3: EXPECTTED_VALUE := 0
dm.3: dm.3.bkoff
cpt.3: cpt
	tail -n 1 $< > $@

dm.3: dm
	ln -fs $< $@


########################################
# fdm.4
# dm starts before cpt.
test.4: EXPECTTED_VALUE := 2
dm.4: dm.4.bkoff
cpt.4: cpt
	egrep -v '^a' $< > $@

dm.4: dm
	ln -fs $< $@


########################################
# fdm.5
# cpt starts before dm.
test.5: EXPECTTED_VALUE := 2
dm.5: dm.5.bkoff
cpt.5: cpt
	ln -fs $< $@

dm.5: dm
	egrep -v '^[aA]' $< > $@


########################################
# fdm.6
# Conditional phrase tables can have more than 3 columns, make sure we filter them ok.
# add a fourth column.
test.6: EXPECTTED_VALUE := 3
dm.6: dm.6.bkoff
cpt.6: cpt
	paste -d ' ' $< <(perl -nle 'print "||| 4"' < $<) > $@

dm.6: dm
	ln -fs $< $@


########################################
# fdm.7
# cpt starts before dm.
test.7.gz: EXPECTTED_VALUE := 2
dm.7.gz: dm.7.bkoff
cpt.7.gz: cpt
	cat $< | gzip > $@

dm.7.bkoff: dm.bkoff
	ln -fs $< $@

dm.7.gz: dm
	egrep -v '^[aA]' $< | gzip > $@

