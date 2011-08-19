#!/usr/bin/make -f
# vim:noet:list

# Makefile - Run this test suite
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

SHELL=bash

# We disable diversitytests at this point because that option is not yet in
# PORTAGEshared.  We copied this unit test suite here anyway, because it is a
# useful test of lattice-related code, and diversity will be added to
# PORTAGEshared eventually.
#all: baselines diversitytests
all: baselines

baselines: base cov2
diversitytests: div2 divincr0 divincr1 divincr20

clean:
	${RM} l?.0000* log.*


base:
	canoe -s 10 -f canoe.ini -lattice lb -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 266) <(wc -l < lb.0000)
	diff <(echo 82) <(wc -l < lb.0000.state)

cov2:
	canoe -s 10 -f canoe.ini -lattice lc -cov-limit 2 -v 2 < input &> log.$@
	diff <(echo 58) <(wc -l < lc.0000)
	diff <(echo 32) <(wc -l < lc.0000.state)

div2:
	canoe -s 10 -f canoe.ini -lattice ld -diversity 2 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 351) <(wc -l < ld.0000)
	diff <(echo 115) <(wc -l < ld.0000.state)

divincr0:
	canoe -s 10 -f canoe.ini -lattice le -diversity 10 -diversity-stack-increment 0 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 47871) <(wc -l < le.0000)
	diff <(echo 5261) <(wc -l < le.0000.state)

divincr1:
	canoe -s 10 -f canoe.ini -lattice lf -diversity 10 -diversity-stack-increment -1 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 723) <(wc -l < lf.0000)
	diff <(echo 170) <(wc -l < lf.0000.state)

divincr20:
	canoe -s 10 -f canoe.ini -lattice lg -diversity 10 -diversity-stack-increment 20 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 1087) <(wc -l < lg.0000)
	diff <(echo 231) <(wc -l < lg.0000.state)
