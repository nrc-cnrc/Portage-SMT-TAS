# @file Makefile
# @brief Testsuite for coarse TMs.
#
# @author Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada


TEMP_FILES=out.jpt.wc out.jpt.wc.bad log.out.jpt.wc.bad \
           out.cpt.rf-cm.en2fr log.out.cpt.rf-cm.en2fr
include ../Makefile.incl

all: out.jpt.wc out.jpt.wc.bad out.cpt.rf-cm.en2fr

out.jpt.wc:
	jpt_word2class jpt.en-fr en.classes fr.classes $@
	diff $@ ref/$@ -q

out.jpt.wc.bad:
	! jpt_word2class jpt.en-fr fr.classes en.classes $@ 2> log.$@ \
	   || ! echo "Error: jpt_word2class did not fail when expected."
	diff log.$@ ref/log.$@ -q
	diff $@ ref/$@ -q

# cpt.wc.en2fr was created by:
#   cat out.jpt.wc |  LC_ALL=C sort -s | merge_counts jpt.wc.en-fr -
#   joint2cond_phrase_tables -v -i -1 en -2 fr -s RFSmoother -multipr fwd -o cpt.wc -no-sort jpt.wc.en-fr

out.cpt.rf-cm.en2fr: %.en2fr:
	joint2cond_phrase_tables -v -reduce-mem -i -1 en -2 fr \
		-s RFSmoother -s "CoarseModel cpt.wc.en2fr en.classes fr.classes" \
		-multipr fwd -no-sort -o $* jpt.en-fr &> log.$@
	diff $@ ref/$@ -q
	