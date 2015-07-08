#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Testsuite for bilm features
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


SHELL=/bin/bash
.PHONY: FORCE
FORCE:

all: out err_min_lmc coarseout boundary_cases

TEMP_FILES=out log.* coarse*out coarse*out.tmp log.coarseout \
           train_en train_fr coarse*_en coarse*_fr \
           coarse*train_bilm coarse*train_bilm.raw \
           en.cls* fr.cls* bitoken*.cls* bilm.coarse*.gz bilm.*gram.gz
TEMP_DIRS=bilm.*.tplm run-p.* lm.tplm
include ../Makefile.incl

init-lm:
	ngram-count -unk -text <(echo .) -lm lm

init-bilm:
	ngram-count -unk -text train_bilm -lm bilm.tiny.gz

out: in canoe.ini bilm.tiny.gz lm
	canoe -f canoe.ini -input in -ffvals > out 2> log.out
	diff out ref/out

err_min_lmc: in canoe.ini lm lm.tplm bilm.tiny.gz bilm.tiny.tplm
	! canoe -f canoe.ini -input in -minimize-lm-context-size >& log.err1
	grep 'Convert LM.*to TPLM format' log.err1
	! canoe -f canoe.ini -input in -minimize-lm-context-size -lmodel-file lm.tplm >& log.err2
	grep 'Convert LM.*to TPLM format' log.err2
	! canoe -f canoe.ini -input in -minimize-lm-context-size -bilm-file bilm.tiny.tplm >& log.err3
	grep 'Convert LM.*to TPLM format' log.err3
	canoe -f canoe.ini -input in -minimize-lm-context-size -lmodel-file lm.tplm -bilm-file bilm.tiny.tplm >& log.noerr
	grep 'Translated ' log.noerr

# Coarse BiLM

.SECONDARY:
refsuffix?=$(or ${cls_en},0)$(or ${cls_fr},0)$(or ${cls_bitoken},0)

##############################################################################
# This prep part of the test suite only works in Portage R&D, not in PortageII
# However, its results are saved in PortageII so we do not need to re-run it.
#
# Before running "make prep":
#  - run the test suite in ../dynamic-pt/
#  - run "make cpt.ibm2.en2fr.gz" in ../dynamic-pt/
TEMP_PREP_FILES=train_en train_fr coarse*_en coarse*_fr \
                coarse*train_bilm en.cls.cats fr.cls.cats \
                train.en2fr.align.gz
clean_prep:
	${RM} ${TEMP_PREP_FILES}

prep: train.en2fr.align.gz \
      coarse${refsuffix}_en coarse${refsuffix}_fr \
      coarse${refsuffix}train_bilm bilm.coarse${refsuffix}.gz \
      coarse.in canoe.ini.coarse.FILT

train_%: train_%.al.bz2
	bzcat $< > $@
%.cls.gz: train_%
	${RM} $@
	mkcls -p$< -V$*.cls
	gzip $*.cls
train.en2fr.align.gz: ../dynamic-pt/train.en2fr.ibm2.tpa
	tp_alignment_dump $< | gzip > $@

coarse.in: ${PORTAGE}/test-suite/tutorial-data/dev1_en.al
	echo 1 2 3 5 6 7 8 9 10 | tr ' ' $$'\n' | lines.py /dev/stdin $< | lc-latin.pl > $@

# in ../dynamic-pt, run "make cpt.ibm2.en2fr.gz"
canoe.ini.coarse.FILT: refsuffix=
canoe.ini.coarse.FILT: coarse.in canoe.ini.coarse ../dynamic-pt/cpt.ibm2.en2fr.gz en.cls.gz fr.cls.gz bilm.coarse.gz
	filter_models -v -f canoe.ini.coarse < $<

lm.tplm: lm
	arpalm2tplm.sh -n 1 $< &> log.$@

############################### End of prep ##################################
##############################################################################


coarse${refsuffix}_%: train_% %.cls.gz
	if [[ -n "${cls_$*}" ]]; then word2class $+ $@; else cp $< $@; fi

checktrain_bilm: train_en train_fr train.en2fr.align.gz
	word_align_tool -fin green -fout bilm -t $+ $@

coarse${refsuffix}train_bilm.raw: coarse${refsuffix}_en coarse${refsuffix}_fr
	word_align_tool -fin green -fout bilm -t $+ train.en2fr.align.gz $@

ifdef cls_bitoken
coarse${refsuffix}train_bilm: coarse${refsuffix}train_bilm.raw bitoken${refsuffix}.cls.gz
	word2class $+ $@
bitoken${refsuffix}.cls.gz: coarse${refsuffix}train_bilm.raw
	${RM} $@
	mkcls -p$< -Vbitoken${refsuffix}.cls
	gzip bitoken${refsuffix}.cls
else
coarse${refsuffix}train_bilm: coarse${refsuffix}train_bilm.raw
	cp $+ $@
endif

bilm.coarse${refsuffix}.gz: coarse${refsuffix}train_bilm
	ngram-count -unk -text $< -lm $@
bilm.%.tplm: bilm.%.gz
	arpalm2tplm.sh -n 1 $< >& log.$@

LM_EXT?=tplm
COARSE_BILM_FILE=bilm.coarse${refsuffix}.${LM_EXT}
ifdef cls_en
   COARSE_BILM_EN=;cls(src)=en.cls.gz
endif
ifdef cls_fr
   COARSE_BILM_FR=;cls(tgt)=fr.cls.gz
endif
ifdef cls_bitoken
   COARSE_BILM_BI=;cls(tgt/src)=bitoken${refsuffix}.cls.gz
endif
COARSE_BILM=${COARSE_BILM_FILE}${COARSE_BILM_EN}${COARSE_BILM_FR}${COARSE_BILM_BI}

LMC_MIN?=""
verbosity ?= 2
coarseout:
	${MAKE} coarse${refsuffix}out LM_EXT=gz LMC_MIN=
	${MAKE} coarse${refsuffix}out LM_EXT=tplm LMC_MIN=
	${MAKE} coarse${refsuffix}out LM_EXT=tplm LMC_MIN=-minimize-lm-context-size

coarse${refsuffix}out: FORCE bilm.coarse${refsuffix}.${LM_EXT} en.cls.gz fr.cls.gz lm.tplm bilm.tiny.${LM_EXT}
	canoe -f canoe.ini.coarse.FILT -ffvals -trace -input coarse.in \
	   -v ${verbosity} -timing ${LMC_MIN} \
	   -bilm-file "bilm.tiny.${LM_EXT}:${COARSE_BILM}" \
	   > $@.tmp 2> log.$@.lmext-${LM_EXT}${LMC_MIN}
	mv $@.tmp $@
	-diff $@ ref/$@ || diff $@ ref/$@-alt


##############################################################################
# Section with toy class files of order up to 16, to test boundary cases

boundary_cases: 7bilm_ok 8bilm_fail 8bilm_nomin_ok \
                15gram_ok 16gram_fail 16gram_nomin_ok

7bilm_ok: override LM_EXT=tplm
7bilm_ok: FORCE coarse.in bilm.tiny.tplm bilm.coarse${refsuffix}.tplm
	canoe -f canoe.ini.coarse.FILT -input coarse.in -v 2 \
	   -minimize-lm-context-size \
	   -bilm-file "bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm" \
	   >& log.$@

8bilm_fail: override LM_EXT=tplm
8bilm_fail: FORCE coarse.in bilm.tiny.tplm bilm.coarse${refsuffix}.tplm
	! canoe -f canoe.ini.coarse.FILT -input coarse.in -v 2 \
	   -minimize-lm-context-size \
	   -bilm-file "bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}" \
	   >& log.$@
	grep 'maximum of 7' log.$@

8bilm_nomin_ok: override LM_EXT=tplm
8bilm_nomin_ok: FORCE coarse.in bilm.tiny.tplm bilm.coarse${refsuffix}.tplm
	canoe -f canoe.ini.coarse.FILT -input coarse.in -v 2 \
	   -no-minimize-lm-context-size \
	   -bilm-file "bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}:bilm.tiny.tplm:${COARSE_BILM}" \
	   >& log.$@

bilm.%gram.gz: coarse${refsuffix}train_bilm
	ngram-count -order $* -unk -text $< -lm $@ >& log.$@

15gram_ok: bilm.15gram.tplm coarse.in bilm.tiny.tplm FORCE
	canoe -f canoe.ini.coarse.FILT -input coarse.in -v 2 \
	   -minimize-lm-context-size \
	   -bilm-file "bilm.tiny.tplm:$<" \
	   >& log.$@

16gram_fail: bilm.16gram.tplm coarse.in bilm.tiny.tplm FORCE
	! canoe -f canoe.ini.coarse.FILT -input coarse.in -v 2 \
	   -minimize-lm-context-size \
	   -bilm-file "bilm.tiny.tplm:$<" \
	   >& log.$@
	grep 'maximum order of 15' log.$@

16gram_nomin_ok: bilm.16gram.tplm coarse.in bilm.tiny.tplm FORCE
	canoe -f canoe.ini.coarse.FILT -input coarse.in -v 2 \
	   -no-minimize-lm-context-size \
	   -bilm-file "bilm.tiny.tplm:$<" \
	   >& log.$@
