# @file Makefile
# @brief Testsuite for coarse LMs.
#
# @author Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

.SECONDARY:

TEMP_FILES=out.wcl_en out.wcl_fr out.wcl_en.bad log.* \
	wcl_en.lm text_en.lm out.lm_eval.text_en out.lm_eval.wcl_en \
	test_en.mapped out.dynmap out.manual \
	out.dynmap_limit out.en.classes.filt \
	word_given_wcl_en.lm word_given_wcl_fr.lm \
	decode.coarse-*
TEMP_DIRS=wcl_en.tplm

include ../Makefile.incl

ifneq ($(shell which ngram-count &> /dev/null && echo 0),0)
$(error Error: we need ngram-count from SRILM for this unittest!)
endif


all: out.wcl_en out.wcl_fr out.wcl_en.bad
all: dynmap_visual_test dynmap_cmp dynmap_limit_cmp
all: out.en.classes.filt
all: word_given_wcl_en.lm word_given_wcl_fr.lm

out.wcl_en out.wcl_fr: out.wcl_%:
	word2class in_$* $*.classes $@
	diff $@ ref/$@ -q

out.wcl_en.bad:
	! word2class in_en fr.classes $@ 2> log.$@ \
	   || ! echo "Error: word2class did not fail when expected."
	diff log.$@ ref/log.$@ -q
	diff $@ ref/$@ -q

wcl_en.lm: out.wcl_en
text_en.lm: in_en
wcl_en.lm text_en.lm: %.lm:
	ngram-count -gt1min 1 -gt2min 1 -gt3min 2 -order 3 -text $< -lm $@

dynmap_visual_test: test_en.txt text_en.lm wcl_en.lm
	lm_eval -v text_en.lm test_en.txt > out.lm_eval.text_en
	lm_eval -v 'DynMap;wordClasses-en.classes;wcl_en.lm' test_en.txt > out.lm_eval.wcl_en
	@echo "*** out.lm_eval.text_en and out.lm_eval.wcl_en are ready for viewing."

dynmap_cmp: EVAL_OPTS=
dynmap_limit_cmp: EVAL_OPTS=-limit

dynmap_cmp dynmap_limit_cmp: %_cmp: test_en.txt wcl_en.lm
	lm_eval -v ${EVAL_OPTS} 'DynMap;wordClasses-en.classes;wcl_en.lm' test_en.txt 2>log.$* | sed 's/.*=//' > out.$*
	word2class -no-error test_en.txt en.classes test_en.mapped
	lm_eval -v wcl_en.lm test_en.mapped | sed 's/.*=//' > out.manual
	cat log.$* | sed 's/[0-9][0-9]*s./0s./; s/[0-9][0-9]* secs/0 secs/' | diff - ref/log.$* -q
	diff out.$* out.manual -q --ignore-matching-lines="Found k-grams"

out.en.classes.filt: en.classes test_en.txt
	filter_word_classes $+ > $@
	diff $@ ref/$@ -q

word_given_wcl_en.lm word_given_wcl_fr.lm: word_given_wcl_%.lm: %.classes in_%
	gen-word-given-wcl-lm.py $+ > $@
	diff $@ ref/$@ -q


###########################################################
# Run some tests decoding using the coarse LM
all: decode fstats
COARSE_LMS = lm tplm min-tplm
decode: ${foreach l,${COARSE_LMS},decode.coarse-$l}
fstats: ${foreach l,${COARSE_LMS},fstats.decode.coarse-$l}

wcl_en.tplm: wcl_en.lm
	arpalm2tplm.sh $< $@ 2> log.$@

decode.coarse-lm: wcl_en.lm cpt.fr2en in_fr canoe.ini
	canoe -f canoe.ini -lmodel-file 'DynMap;wordClasses-en.classes;$<' -input in_fr  -v 2 > $@ 2> log.$@

decode.coarse-tplm: wcl_en.tplm cpt.fr2en in_fr canoe.ini
	canoe -f canoe.ini -lmodel-file 'DynMap;wordClasses-en.classes;$<' -input in_fr  -v 2 > $@ 2> log.$@

decode.coarse-min-tplm: wcl_en.tplm cpt.fr2en in_fr canoe.ini
	canoe -f canoe.ini -lmodel-file 'DynMap;wordClasses-en.classes;$<' -input in_fr  -minimize-lm-context-size -v 2 > $@ 2> log.$@

fstats.%: %
	egrep 'FSTATS|recombined' log.$* | diff - ref/log.$*.fstats
