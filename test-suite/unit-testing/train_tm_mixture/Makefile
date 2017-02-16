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
all: test1 test2 test3 test4 test_PI all_ac_fields

TEMP_FILES=log.* out.* dyn.mixtm trivial.lm triang.cpt*
TEMP_DIRS=cpt*.tppt* cpt.mix.test*
include ../Makefile.incl

export PORTAGE_INTERNAL_CALL=1

.PHONY: test1 test2 test3 test4 test_PI

test1: MIX_OPTS =

test2: MIX_OPTS = -w 2,0.1,0.9

test3: MIX_OPTS = -w 1,0.1,0.9 -w 2,0.2,0.8 -w 3,0.7,0.3 -w 4,0.6,0.4

test4: MIX_OPTS = -w 1,0.2,0.8 -prec .0001

test1 test2 test3 test4: %: in/cpt.hans1.en2fr.gz in/cpt.hans2.en2fr.gz in/jpt.dev.en-fr.gz
	train_tm_mixture -v ${MIX_OPTS} -o cpt.mix.$*.en2fr.gz -dynout cpt.mix.$*.en2fr $+ >& log.cpt.mix.$*.en2fr
	diff-round.pl log.cpt.mix.$*.en2fr ref/log.cpt.mix.$*.en2fr -q
	diff-round.pl cpt.mix.$*.en2fr.gz ref/cpt.mix.$*.en2fr.gz -q

# test_PI adds a cpt with a fourth column, that should simply be ignored and dropped.
test_PI: %: in/cpt.hans1.en2fr.gz in/cpt.hans2.en2fr.gz in/cpt.PI.en2fr.gz in/jpt.dev.en-fr.gz
	! { train_tm_mixture -v ${MIX_OPTS} -o cpt.mix.$*.en2fr.gz $+ >& log.cpt.mix.$*.en2fr; }
	grep -q 'incorrect format in phrase table' log.cpt.mix.$*.en2fr


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

######################################
# train_tm_mixture generating the a= and c= fields

.PHONY: all_ac_fields
all_ac_fields:
	${MAKE} ac_fields AC_PREFIX=no AC_OPTS=""
	${MAKE} ac_fields AC_PREFIX=a AC_OPTS="-write-al top"
	${MAKE} ac_fields AC_PREFIX=c AC_OPTS="-write-count"
	${MAKE} ac_fields AC_PREFIX=ac AC_OPTS="-write-al top -write-count"

.PHONY: ac_fields
ac_fields: _fields.normal _fields.reversed _fields.swap

AC_PREFIX=ac
AC_OPTS=-write-count -write-al top

_fields.normal:
	train_tm_mixture ${AC_OPTS} -o out.${AC_PREFIX}$@ in/cpt-1b in/cpt-2b in/cpt-3 in/jpt
	-diff out.${AC_PREFIX}$@ ref/out.${AC_PREFIX}_fields

_fields.swap:
	train_tm_mixture ${AC_OPTS} -o out.${AC_PREFIX}$@ in/cpt-2b in/cpt-1b in/cpt-3 in/jpt
	-diff out.${AC_PREFIX}$@ ref/out.${AC_PREFIX}_fields

# We gotta be able to parse a= and c= in either direction, really! (We didn't use to).
_fields.reversed:
	train_tm_mixture ${AC_OPTS} -o out.${AC_PREFIX}$@ in/cpt-1 in/cpt-2 in/cpt-3 in/jpt
	diff out.${AC_PREFIX}$@ ref/out.${AC_PREFIX}_fields


######################################
# train_tm_mixture generating just the config file for a dynamic mixtm

all: test_dyn_mixtm

test_dyn_mixtm:
	train_tm_mixture -dynout dyn -v in/cpt-1 in/cpt-2 in/cpt-3 in/jpt >& log.$@
	diff dyn.mixtm ref/

######################################
# decode with a mixture model as a dynamic MixTM vs a static mix one

all: test_decoding diff_decoding
test_decoding: decode.ttmp.cpt.mix.test1.en2fr.gz
test_decoding: decode.tt.cpt.mix.test1.en2fr.tppt
test_decoding: decode.tt.cpt.mix.test1.en2fr.tppt.mixtm

cpt.mix.test1.en2fr.gz: test1
cpt.mix.test1.en2fr.mixtm: test1

diff_decoding: test_decoding
	diff-round.pl -sort triang.cpt.mix.test1.en2fr.gz triang.cpt.mix.test1.en2fr.tppt
	diff-round.pl -p 5 -sort triang.cpt.mix.test1.en2fr.gz triang.cpt.mix.test1.en2fr.tppt.mixtm

cpt%.tppt: cpt%.gz
	textpt2tppt.sh $< >& log.$@

cpt%.tppt.mixtm: cpt%.mixtm
	perl -ple 's/in\/(.*?)(\.gz)?\t/\1.tppt\t/' < $< > $@
	for CPT in `grep -o 'in\/.*\.gz' $<`; do textpt2tppt.sh $$CPT; done >& log.$@

trivial.lm:
	echo ceci est un test . | estimate-ngram -text /dev/stdin -write-lm $@

decode.tt.%: % trivial.lm
	canoe -f /dev/null -lmodel-file trivial.lm -ttable $* -triangularArrayFilename triang.$* <<<'this is a test .' 2> log.$@

decode.ttmp.%: % trivial.lm
	canoe -f /dev/null -lmodel-file trivial.lm -ttable-multi-prob $* -triangularArrayFilename triang.$* <<<'this is a test .' 2> log.$@

all: test_errors
test_errors: multiprob_in_ttable
test_errors: multiprob_in_mixtm

# Comment out to see the error messages from each case below
QUIET_TEST_ERRORS=-q

multiprob_in_ttable: cpt.mix.test1.en2fr.gz trivial.lm
	canoe -f /dev/null -lmodel-file trivial.lm -ttable $< <<<'test' |& grep ${QUIET_TEST_ERRORS} Error:

multiprob_in_mixtm: cpt.mix.test1.en2fr.mixtm trivial.lm
	canoe -f /dev/null -lmodel-file trivial.lm -ttable $< <<<'test' |& grep ${QUIET_TEST_ERRORS} Error:
