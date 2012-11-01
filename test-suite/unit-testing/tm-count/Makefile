# vim:noet:list
# Makefile - Test handling counts in phrase tables.
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


all: test

test: test-a test-ca test-c
	@echo All tests PASSED.

TEMP_FILES=log.* 1sent.out-* *.fr2en.gz \
           canoe.ini-* \
           1sent.out-* 1sent.out.2-* \
           canoe.ini-*.*FILT* *-cpt.*.FILT* cpt.*.FILT-G-* \
           canoe.ini-*.ONE* *-cpt.online-input.ONE*.gz
include ../Makefile.incl

clean: clean-deep
clean-deep:
	find . -type l -name \*.fr2en.gz | xargs ${RM}


test-%: configtool-% canoe-% filter_models-%
	@echo $@ passed.

canoe.ini-a canoe.ini-ca canoe.ini-c: canoe.ini-%: canoe.ini.template
	sed "s/OPT/$*/" < $< > $@

a-cpt.fr2en.gz:
	-ln -s ../gen_phrase_tables/ref/cpt.top.fr2en.gz $@

%.fr2en.gz:
	-ln -s ../gen_phrase_tables/ref/$*.fr2en.gz


# configtool

configtool-%: check-% nf-% weights-%
	@echo $@ passed.

check-%: %-cpt.fr2en.gz canoe.ini-%
	configtool check canoe.ini-$*

nf-%: check-%
	configtool nf canoe.ini-$* | diff - <(echo 12)

weights-%: check-%
	configtool weights canoe.ini-$* | diff - <(echo ' -d 1 -w 0 -unal 1:1:1:1:1 -lm 1 -tm 1 -ftm 1 -atm 1:1')


# Decoding

canoe-%: cmp_1sent.out-% cmp_1sent.out.2-%
	@echo $@ passed.

1sent.out-%: 1sent check-%
	canoe -f canoe.ini-$* -trace -ffvals < $< > $@ 2> log.$@

1sent.out.2-%: 1sent check-%
	canoe -f canoe.ini-$* -load-first -trace -ffvals < $< > $@ 2> log.$@


# Filtering

filter_models-%: filter_grep-% filter_hard-% filter_soft-% filter_hard_online-% filter_soft_online-%
	@echo $@ passed.

filter_grep-%:
	filter_models -f canoe.ini-$* -suffix .FILT-G-$* < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_canoe.ini-$*.FILT-G-$* cmp_cpt.4thonly.FILT-G-$* cmp_cpt.alonly.FILT-G-$* cmp_$*-cpt.fr2en.FILT-G-$*.gz

filter_hard-%:
	filter_models -c -ttable-limit 2 -f canoe.ini-$* -tm-hard-limit $*-cpt.hard < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_$*-cpt.hard.FILT

filter_soft-%:
	filter_models -c -ttable-limit 2 -f canoe.ini-$* -tm-soft-limit $*-cpt.soft < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_$*-cpt.soft.FILT

%-cpt.online-input.ONECPT.gz:
	filter_models -ttable-limit 100 -f canoe.ini-$* -tm-soft-limit $*-cpt.online-input -no-src-grep -z -suffix .ONECPT 2> log.$@

filter_hard_online-%: %-cpt.online-input.ONECPT.gz
	filter_models -tm-online -ttable-limit 2 -f canoe.ini-$*.ONECPT -tm-hard-limit $*-cpt.hard.online < 1sent
	@${MAKE} --no-print-directory cmp_$*-cpt.hard.online.FILT

filter_soft_online-%: %-cpt.online-input.ONECPT.gz
	filter_models -tm-online -ttable-limit 2 -f canoe.ini-$*.ONECPT -tm-soft-limit $*-cpt.soft.online < 1sent
	@${MAKE} --no-print-directory cmp_$*-cpt.soft.online.FILT

cmp_%: %
	diff-round.pl -sort $* ref/$* -q

