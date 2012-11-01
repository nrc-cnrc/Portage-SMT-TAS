# @file Makefile runs bad LM through arpalm2binlm and back as filter
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

all: test_filtering
	@echo All tests PASSED

test_filtering: bad.filteredlm
	diff-round.pl -sort $< ref/$<

TEMP_FILES=bad.binlm bad.filteredlm
include ../Makefile.incl

%.binlm: %.lm
	arpalm2binlm $< $@

%.filteredlm: %.binlm
	binlm2arpalm $< $@


