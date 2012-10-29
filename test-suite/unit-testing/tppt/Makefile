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

all: mix12 mix56 round_trip detect-bad
	@echo All tests PASSED.

mix12: cmp_cpt_cpt cmp_cpt_tppt cmp_tppt_cpt cmp_tppt_tppt

mix56: cmp_cpt5_cpt6 cmp_cpt5_tppt6 cmp_tppt5_tppt6

round_trip: cpt1.diff cpt2.diff cpt3.diff cpt4.diff cpt7.diff

.SECONDARY:

clean:
	${RM} -r log.* cpt*.tppt cpt*tppt.tmp.* cpt?.dump *.0000.10best*

cmp_%: %
	diff $*.0000.10best ref/
	diff $*.0000.10best.ffvals ref/

%.tppt: %
	textpt2tppt.sh -v $< >& log.$@

cpt_cpt: canoe.ini.cpt_cpt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

cpt_tppt: cpt2.tppt canoe.ini.cpt_tppt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

tppt_cpt: cpt1.tppt canoe.ini.tppt_cpt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

tppt_tppt: cpt1.tppt cpt2.tppt canoe.ini.tppt_tppt
	echo a b c d | canoe -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

cpt5_cpt6: canoe.ini.cpt5_cpt6
	echo a b c d | canoe -v 1 -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

cpt5_tppt6: canoe.ini.cpt5_tppt6 cpt6.tppt
	echo a b c d | canoe -v 1 -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

tppt5_tppt6: canoe.ini.tppt5_tppt6 cpt5.tppt cpt6.tppt
	echo a b c d | canoe -v 1 -f canoe.ini.$@ -ffvals -nbest $@:10 >& log.$@

cpt%.dump: cpt%.tppt
	tppt.dump $< > $@

cpt%.diff: cpt% cpt%.dump
	diff-round.pl -p 5 $+

cpt4.diff: ref/cpt4.fillcounts cpt4.dump
	diff-round.pl -p 5 $+

cpt7.diff: ref/cpt7.dump cpt7.dump
	diff-round.pl -p 5 $+

detect-bad: cpt-bad
	! textpt2tppt.sh -v $< >& log.$@

