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

all: base cov2 div2 divincr0 divincr1 divincr20

#STACKOPT=-s
#CANOE=/home/joanise/sandboxes/PortageTrunk/src/canoe/canoe
STACKOPT=-rs
CANOE=canoe
TEMP_FILES=l?.0000* log.*
include ../Makefile.incl

base:
	${CANOE} ${STACKOPT} 10 -f canoe.ini -lattice lb -diversity 0 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 266) <(wc -l < lb.0000)
	diff <(echo 82) <(wc -l < lb.0000.state)

cov2:
	${CANOE} ${STACKOPT} 10 -f canoe.ini -lattice lc -diversity 0 -cov-limit 2 -v 2 < input &> log.$@
	diff <(echo 56) <(wc -l < lc.0000)
	diff <(echo 31) <(wc -l < lc.0000.state)

div2:
	${CANOE} ${STACKOPT} 10 -f canoe.ini -lattice ld -diversity 2 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 365) <(wc -l < ld.0000)
	diff <(echo 118) <(wc -l < ld.0000.state)

divincr0:
	${CANOE} ${STACKOPT} 10 -f canoe.ini -lattice le -diversity 10 -diversity-stack-increment 0 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 47871) <(wc -l < le.0000)
	diff <(echo 5261) <(wc -l < le.0000.state)

divincr1:
	${CANOE} ${STACKOPT} 10 -f canoe.ini -lattice lf -diversity 10 -diversity-stack-increment -1 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 707) <(wc -l < lf.0000)
	diff <(echo 172) <(wc -l < lf.0000.state)

divincr20:
	${CANOE} ${STACKOPT} 10 -f canoe.ini -lattice lg -diversity 10 -diversity-stack-increment 20 -cov-limit 0 -v 2 < input &> log.$@
	diff <(echo 1087) <(wc -l < lg.0000)
	diff <(echo 231) <(wc -l < lg.0000.state)
