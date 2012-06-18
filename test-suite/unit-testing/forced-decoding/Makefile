# vim:noet:list
# Makefile - Test forced decoding
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

SHELL=bash
.SECONDARY:

all: canoe_all emptyjpt
	@echo All tests PASSED.

canoe_all: check_canoe-small check_canoe-long

clean:
	${RM} small.* long.* log.* cpt.top.fr2en.gz

emptyjpt: small_fr small_en
	! canoe -forced -f canoe.ini -ttable-multi-prob /dev/null -ref small_fr < small_en
	@echo Bad input \(empty phrase table\) test PASSED.

cpt.top.fr2en.gz:
	zcat -f ../gen_phrase_tables/ref/cpt.top.fr2en.gz | \
	   perl -pe 's/ 1 1 / 0.9 0.8 /' | gzip > cpt.top.fr2en.gz

canoe-%: cpt.top.fr2en.gz %_en %_fr canoe.ini.small
	canoe -forced -f canoe.ini.small -append -nbest $*:10 -trace -ffvals \
	   -ref $*_en < $*_fr > $*.canoe

check_canoe-%: canoe-%
	diff $*.canoe ref/$*.canoe -q
	diff $*.10best ref/$*.10best -q
	diff-round.pl -q $*.10best.ffvals ref/$*.10best.ffvals
	diff $*.10best.pal ref/$*.10best.pal -q

