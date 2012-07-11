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
all: gen jpt freq bad
	@echo All tests PASSED

.PHONY: gen jpt freq
gen jpt freq:
	${MAKE} -j 1 -C $@

clean:
	${MAKE} -C gen clean
	${MAKE} -C jpt clean
	${MAKE} -C freq clean

SHELL=/bin/bash

# this test still fails, so don't activate it yet.
empty:
	merge_counts empty.gz /dev/null
	(( `wc -c < empty.gz` == 20 ))

bad:
	! merge_counts bad-out.gz bad.gz
	@echo bad test passed: bad.gz is indeed corrupt
