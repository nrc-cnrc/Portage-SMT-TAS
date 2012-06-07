# $Id$
#
# @file Makefile to test train_tm_mixture
# @author Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

.SECONDARY:

.PHONY: all
all: test1 test2 test3 test4
	@echo All tests PASSED.

.PHONY: clean
clean:
	${RM} cpt.mix.test* log.cpt.mix.test*

export PORTAGE_INTERNAL_CALL=1

.PHONY: test1 test2 test3 test4

test1: MIX_OPTS =

test2: MIX_OPTS = -w 2,0.1,0.9

test3: MIX_OPTS = -w 1,0.1,0.9 -w 2,0.2,0.8 -w 3,0.7,0.3 -w 4,0.6,0.4

test4: MIX_OPTS = -w 1,0.2,0.8 -prec .0001

test1 test2 test3 test4: %: in/cpt.hans1.en2fr.gz in/cpt.hans2.en2fr.gz in/jpt.dev.en-fr.gz
	train_tm_mixture -v ${MIX_OPTS} -o cpt.mix.$*.en2fr.gz $+ >& log.cpt.mix.$*.en2fr
	diff-round.pl log.cpt.mix.$*.en2fr ref/log.cpt.mix.$*.en2fr -q
	diff-round.pl cpt.mix.$*.en2fr.gz ref/cpt.mix.$*.en2fr.gz -q


########################################
# HELPERS

.PHONY: pts
pts: cpt.hans1.en2fr.gz cpt.hans2.en2fr.gz jpt.dev.en-fr.gz

.PHONY: clean.pts
clean.pts:
	${RM} ibm{1,2}.hans*.{en_given_fr,fr_given_en}* cpt.hans* jpt.dev*
	${RM} dev_{en,fr}.lc
	${RM} log.ibm2.hans* log.cpt.hans* log.jpt.dev.*

cpt.hans1.en2fr.gz cpt.hans2.en2fr.gz: cpt.%.en2fr.gz: 
	train_ibm -v  -bin -s ibm1.$*.fr_given_en.gz ibm2.$*.fr_given_en.gz \
		in/$*_en.lc.gz in/$*_fr.lc.gz >& log.ibm2.$*.fr_given_en
	train_ibm -vr -bin -s ibm1.$*.en_given_fr.gz ibm2.$*.en_given_fr.gz \
		in/$*_en.lc.gz in/$*_fr.lc.gz >& log.ibm2.$*.en_given_fr
	gen_phrase_tables -v -z -m8 -1 en -2 fr -multipr fwd \
		-s "KNSmoother 3" -s ZNSmoother -o cpt.$*.unsorted \
		ibm2.$*.fr_given_en.gz ibm2.$*.en_given_fr.gz \
		in/$*_en.lc.gz in/$*_fr.lc.gz \
		>& log.cpt.$*.en2fr
	zcat cpt.$*.unsorted.en2fr.gz | LC_ALL=C sort -s | gzip > $@

jpt.dev.en-fr.gz: jpt.%.en-fr.gz: cpt.hans1.en2fr.gz
	zcat in/hans3_en.lc.gz | head -50 > dev_en.lc
	zcat in/hans3_fr.lc.gz | head -50 > dev_fr.lc
	gen_phrase_tables -v -d6 -m7 -1 en -2 fr -j \
		ibm2.hans1.fr_given_en.gz ibm2.hans1.en_given_fr.gz $*_en.lc $*_fr.lc \
		2> log.jpt.dev.en-fr \
		| LC_ALL=C sort -s | gzip > $@
