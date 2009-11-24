#!/ust/bin/make -f
# vim:noet:ts=3
#
# @file Makefile run this test suite
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

.PHONY: all
all: gen jpt freq
	@echo All tests PASSED

.PHONY: gen jpt freq
gen jpt freq:
	${MAKE} -j 1 -C $@

clean:
	${MAKE} -C gen clean

#${MAKE} -j 1 -C gen
#${MAKE} -j 1 -C jpt
#${MAKE} -j 1 -C freq
