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

test: test-a test-ca test-c test-tp

TEMP_FILES=log.* 1sent.out-* *.fr2en.gz \
           canoe.ini.tp-* 1sent.out.tp-* \
           canoe.ini-* \
           1sent.out-* 1sent.out.2-* \
           canoe.ini-*.*FILT* *-cpt.*.FILT* cpt.*.FILT-G-* \
           canoe.ini-*.ONE* *-cpt.online-input.ONE*.gz
TEMP_DIRS=*cpt.*.tppt*
include ../Makefile.incl

clean: clean-deep
clean-deep:
	find . -type l -name \*.fr2en.gz | xargs ${RM}

test-tp: check.tp-1sent.out.tp-a check.tp-1sent.out.tp-c check.tp-1sent.out.tp-ca
test-tp: check.tp-1sent.out.tp-a.ONECPT check.tp-1sent.out.tp-c.ONECPT check.tp-1sent.out.tp-ca.ONECPT

test-%: configtool-% canoe-% filter_models-%
	@echo $@ passed.

canoe.ini-a canoe.ini-ca canoe.ini-c: canoe.ini-%: canoe.ini.template %-cpt.fr2en.gz
	sed "s/OPT/$*/" < $< > $@

a-cpt.fr2en.gz:
	ln -s ../gen_phrase_tables/ref/cpt.top.fr2en.gz $@ || true

%.fr2en.gz:
	ln -s ../gen_phrase_tables/ref/$*.fr2en.gz || true


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
	canoe -f canoe.ini-$* -trace -walign -ffvals < $< > $@ 2> log.$@

1sent.out.2-%: 1sent check-%
	canoe -f canoe.ini-$* -load-first -trace -walign -ffvals < $< > $@ 2> log.$@


# Tight packing

canoe.ini.tp-%.ONECPT: canoe.ini-%.ONECPT
	configtool -p tp $< $@
	for x in `configtool list-tm $<`; do textpt2tppt.sh $$x; done
	configtool check $@

canoe.ini.tp-%: canoe.ini-%
	configtool -p tp $< $@
	for x in `configtool list-tm $<`; do textpt2tppt.sh $$x; done
	configtool check $@

1sent.out.tp-%: 1sent canoe.ini.tp-%
	canoe -f canoe.ini.tp-$* -load-first -trace -walign -ffvals < $< > $@ 2> log.$@

check.tp-1sent.out.tp-%: 1sent.out.tp-% 1sent.out-%
	diff-round.pl $+ -q


# Filtering

filter_models-%: filter_grep-% filter_hard-% filter_soft-% filter_hard_online-% filter_soft_online-%
	@echo $@ passed.

filter_grep-%: canoe.ini-%
	filter_models -f $< -suffix .FILT-G-$* < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_$<.FILT-G-$* cmp_cpt.4thonly.FILT-G-$* cmp_cpt.alonly.FILT-G-$* cmp_$*-cpt.fr2en.FILT-G-$*.gz

filter_hard-%: canoe.ini-%
	filter_models -c -ttable-limit 2 -f $< -tm-hard-limit $*-cpt.hard < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_$*-cpt.hard.FILT

filter_soft-%: canoe.ini-%
	filter_models -c -ttable-limit 2 -f $< -tm-soft-limit $*-cpt.soft < 1sent 2> log.$@
	@${MAKE} --no-print-directory cmp_$*-cpt.soft.FILT

canoe.ini-%.ONECPT: canoe.ini-%
	filter_models -ttable-limit 100 -f $< -tm-soft-limit $*-cpt.online-input -no-src-grep -z -suffix .ONECPT 2> log.$@

filter_hard_online-%: canoe.ini-%.ONECPT
	filter_models -c -tm-online -ttable-limit 2 -f canoe.ini-$*.ONECPT -tm-hard-limit $*-cpt.hard.online < 1sent
	@${MAKE} --no-print-directory cmp_$*-cpt.hard.online.FILT

filter_soft_online-%: canoe.ini-%.ONECPT
	filter_models -c -tm-online -ttable-limit 2 -f canoe.ini-$*.ONECPT -tm-soft-limit $*-cpt.soft.online < 1sent
	@${MAKE} --no-print-directory cmp_$*-cpt.soft.online.FILT

cmp_%: %
	diff-round.pl -sort $* ref/$* -q || diff-round.pl -sort $* ref/$*-alt -q

