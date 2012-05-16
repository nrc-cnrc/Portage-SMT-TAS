# vim:noet:list
# Makefile - Test for combining multi-prob TMs and TPPTs
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

all: cmp_cpt_cpt cmp_cpt_tppt cmp_tppt_cpt cmp_tppt_tppt
	@echo All tests PASSED.

clean:
	${RM} -r log.* cpt?.tppt *.0000.10best*

cmp_%: %
	diff $*.0000.10best ref/
	diff $*.0000.10best.ffvals ref/

%.tppt: %
	textpt2tppt.sh $<

cpt_cpt: canoe.ini.cpt_cpt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

cpt_tppt: cpt2.tppt canoe.ini.cpt_tppt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

tppt_cpt: cpt1.tppt canoe.ini.tppt_cpt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

tppt_tppt: cpt1.tppt cpt2.tppt canoe.ini.tppt_tppt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@
