#!/usr/bin/make -f
# vim:noet:ts=3
# Makefile - Test generation of TPPTs.
#
# PROGRAMMER: Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

.SECONDARY:

.PHONY: all
all: test_general test_float_limit test_sizes
	@echo All tests PASSED \(assuming you ran with -B\)


# Convenient target to run the test using the executable in ${SANDBOX}
SANDBOX ?= ~/workspace/Portage-64/src/tpt
sandbox:
	@echo "Using: `PATH=${SANDBOX}:$$PATH which textpt2tppt.sh`"
	PATH=${SANDBOX}:$$PATH ${MAKE} all -B | egrep --line-buffered -v 'make.*(ing directory)'
	
sandbox_%:
	@echo "Using: `PATH=${SANDBOX}:$$PATH which textpt2tppt.sh`"
	PATH=${SANDBOX}:$$PATH ${MAKE} $* -B | egrep --line-buffered -v 'make.*(ing directory)'


.PHONY: clean
clean:
	${RM} -r pt.tppt* float_limit_pt.tppt* small_pt.test.*.tppt*
	${RM} log.text_pt log.float_limit_pt log.small_pt.test.*
	${RM} small_pt.test.*


# The following commands are executed by textpt2tppt.sh:
#   ptable.encode-phrases textpt 1 xxx
#   ptable.encode-phrases textpt 2 xxx
#   ptable.encode-scores textpt xxx
#   ptable.assemble xxx

.PHONY: test_general
test_general:
	textpt2tppt.sh text_pt pt >&log.text_pt
#	diff -q log.text_pt ref/log.text_pt
	tppt.dump pt.tppt | diff-round.pl -q -sort - text_pt

# We've added some robustness since the scores/probs are read as float they
# need to be within a certain range where min ~ 1E-37 and max ~ 1E+37.
.PHONY: test_float_limit
test_float_limit: float_limit_pt.tppt
	tppt.dump float_limit_pt.tppt | diff-round.pl -q -sort - ref/float_limit_pt


# Test a sequence of small sizes to test boundary conditions.
.PHONY: test_sizes
test_sizes: $(addprefix test_size., 1 2 3 4 5 6 7 8 9 10 11)

test_size.%: small_pt.test.%.tppt
	tppt.dump small_pt.test.$*.tppt | diff-round.pl -q -sort - small_pt.test.$*

$(addprefix small_pt.test., 1 2 3 4 5 6 7 8 9 10 11): small_pt.test.%:
	head -$* small_pt >small_pt.test.$*

# Generic target to build tppt.
%.tppt: %
	${RM} -r $<.tppt
	textpt2tppt.sh $< >&log.$<
