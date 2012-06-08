# vim:noet:list
# Makefile - Test decoding using the unal feature.
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

SHELL=bash

all: configtool canoe filter_models
	@echo All tests PASSED.

clean: clean_canoe clean_filter
	${RM} cpt.top.fr2en.gz log.*

cpt.top.fr2en.gz:
	-ln -s ../gen_phrase_tables/ref/cpt.top.fr2en.gz


configtool: check nf weights

check: cpt.top.fr2en.gz
	configtool check canoe.ini

nf:
	configtool nf canoe.ini | diff - <(echo 12)

weights:
	configtool weights canoe.ini | diff - <(echo ' -d 1 -w 0 -unal 1:1:1:1:1 -lm 1 -tm 1 -ftm 1 -atm 1:1')


canoe: cmp_1sent.out cmp_1sent.out.2

clean_canoe:
	${RM} 1sent.out 1sent.out.2

1sent.out: 1sent cpt.top.fr2en.gz
	canoe -f canoe.ini -trace -ffvals < $< > $@ 2> log.$@

1sent.out.2: 1sent cpt.top.fr2en.gz
	canoe -f canoe.ini -load-first -trace -ffvals < $< > $@ 2> log.$@


filter_models: filter_grep filter_hard filter_soft filter_hard_online filter_soft_online

clean_filter:
	${RM} canoe.ini.*FILT* cpt.*.FILT*
	${RM} canoe.ini.ONE* cpt.online-input.ONE*.gz

filter_grep:
	filter_models -f canoe.ini -suffix .FILT-G < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_canoe.ini.FILT-G cmp_cpt.4thonly.FILT-G cmp_cpt.alonly.FILT-G cmp_cpt.top.fr2en.FILT-G.gz

filter_hard:
	filter_models -ttable-limit 2 -f canoe.ini -tm-hard-limit cpt.hard < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_cpt.hard.FILT

filter_soft:
	filter_models -ttable-limit 2 -f canoe.ini -tm-soft-limit cpt.soft < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_cpt.soft.FILT

cpt.online-input.ONECPT.gz:
	filter_models -ttable-limit 100 -f canoe.ini -tm-soft-limit cpt.online-input -no-src-grep -z -suffix .ONECPT 2> log.$@

filter_hard_online: cpt.online-input.ONECPT.gz
	filter_models -tm-online -ttable-limit 2 -f canoe.ini.ONECPT -tm-hard-limit cpt.hard.online < 1sent
	@${MAKE} --no-print-directory cmp_cpt.hard.online.FILT

filter_soft_online: cpt.online-input.ONECPT.gz
	filter_models -tm-online -ttable-limit 2 -f canoe.ini.ONECPT -tm-soft-limit cpt.soft.online < 1sent
	@${MAKE} --no-print-directory cmp_cpt.soft.online.FILT

cmp_%: %
	diff-round.pl -sort $* ref/$* -q

