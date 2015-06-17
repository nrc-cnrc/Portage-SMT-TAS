# @file Makefile Produces all kinds of filtering and make sure they are equivalent.
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

SHELL = bash

export PORTAGE_INTERNAL_CALL=1
FILTER_MODELS := filter_models
HEAD_SIZE ?= 100
FILES     ?= online.hard.complete online.hard.incomplete online.soft.complete online.soft.incomplete notonline.hard.complete notonline.hard.incomplete notonline.soft.complete notonline.soft.incomplete
CLEAN_PATH ?= ~/sandBoxes/CleanVersion/Portage/src/canoe
#PORTAGE ?= /home/portage
PORTAGE ?= ../../..


.PHONY: clean small check all online online_hard online_soft notonline notonline_hard notonline_soft FORCE
.PRECIOUS: $(foreach f, $(FILES), trans.$f)

all: gitignore small test-dm prune-type-full check phrases-GT-ZN.fr2en.FILT.gz

############################################################
## CLEAN
TEMP_FILES=log.* core.* tm.*.ttpt-* \
           notonline* online* \
           europarl.en.srilm.FILT \
           phrases-GT-ZN.fr2en.FILT.gz phrases-GT-ZN.fr2en.ORIG.GREP.gz phrases-GT-ZN.fr2en.ORIG.LIMIT.gz \
           phrases-GT-KN.fr2en.FILT.gz phrases-GT-KN.fr2en.ORIG.GREP.gz phrases-GT-KN.fr2en.ORIG.LIMIT.gz \
           canoe.ini* phrases.small.* dm.small* trans.* tmp.* prune-full.*

clean:
	${RM} ${TEMP_FILES}

SETUP_FILES= src ibm[12].* phrases-GT-ZN.fr2en.gz .gitignore
distclean: clean
	[ ! -a src ] || chmod +w src
	${RM} ${SETUP_FILES}

gitignore:
	echo "${TEMP_FILES} ${SETUP_FILES}" | tr ' ' '\n' > .gitignore

############################################################
## SETUP
ibm2.fr_given_en.gz:
	LANG=en_US.ISO-8859-1; \
	train_ibm -vr -s       \
	  ibm1.fr_given_en.gz  \
	  ibm2.fr_given_en.gz  \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.fr.lowercase \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.en.lowercase \
	  >& log.train_ibm.fr_given_en

ibm2.en_given_fr.gz:
	LANG=en_US.ISO-8859-1; \
	train_ibm -v  -s       \
	  ibm1.en_given_fr.gz  \
	  ibm2.en_given_fr.gz  \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.fr.lowercase \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.en.lowercase \
	  >& log.train_ibm.en_given_fr

phrases-GT-ZN.fr2en.gz: ibm2.fr_given_en.gz ibm2.en_given_fr.gz
	gen_phrase_tables -v -w1 -m8 -1 fr -2 en -ibm 2 -z \
	  -s GTSmoother -s ZNSmoother   \
	  -multipr fwd                  \
	  -o phrases-GT-ZN              \
	  ibm2.en_given_fr.gz           \
	  ibm2.fr_given_en.gz           \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.fr.lowercase \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.en.lowercase \
	  >& log.gen_multi_prob_phrase_tables

canoe.ini:
	env echo -e "[ttable-multi-prob] \n \
	phrases-GT-ZN.fr2en.gz\n \
	[lmodel-file]\n \
	${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm\n \
	[weight-d] 0.3200787604\n \
	[weight-w] 0.3201824725\n \
	[weight-l] 1\n \
	[weight-t] 0.4:1\n \
	[ttable-limit] 30\n \
	[ttable-prune-type] backward-weights\n\
	[distortion-limit] 7\n \
	[distortion-model] WordDisplacement\n" > $@

canoe.ini.ttpt-%: canoe.ini phrases-GT-ZN.fr2en.gz phrases-GT-ZN.fr2en.gz
	configtool -c args:"-ttable-multi-prob phrases-GT-ZN.fr2en.gz \
	  -weight-f 0.8:0.01 -ttable-limit 4 \
	  -ttable-prune-type $* " $< > $@

canoe.ini_:
	env echo -e "[lmodel-file]\n \
	${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm\n \
	[weight-d] 0.3200787604\n \
	[weight-w] 0.3201824725\n \
	[weight-l] 1\n \
	[weight-t] 0.4:1\n \
	[ttable-limit] 30\n \
	[distortion-limit] 7\n \
	[distortion-model] WordDisplacement\n" > $@

SETUP: phrases-GT-ZN.fr2en.gz canoe.ini canoe.ini_


############################################################
## TREE AND LEAVES
leaves: online notonline
online: online_hard online_soft
online_hard: online.hard.complete online.hard.incomplete
online_soft: online.soft.complete online.soft.incomplete

notonline: notonline_hard notonline_soft
notonline_hard: notonline.hard.complete notonline.hard.incomplete
notonline_soft: notonline.soft.complete notonline.soft.incomplete


############################################################
## SOURCE
src:
	head -n $(HEAD_SIZE) ${PORTAGE}/test-suite/regress-small-voc/lc/test2000.fr.lowercase > $@
	chmod 444 $@



############################################################
## GENERATING PHRASE TABLES
FILT_CMD := ${FILTER_MODELS} -c -f canoe.ini -suffix ".NEW"
online.hard.complete: canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -no-src-grep -tm-online -tm-hard-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

online.hard.incomplete: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -tm-online -tm-hard-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

online.soft.complete: canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -no-src-grep -tm-online -tm-soft-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

online.soft.incomplete: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -tm-online -tm-soft-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@


notonline.hard.complete: canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -no-src-grep -tm-hard-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

notonline.hard.incomplete: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -tm-hard-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

notonline.soft.complete: canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -no-src-grep -tm-soft-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

notonline.soft.incomplete: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(FILT_CMD) -tm-soft-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW; } &> log.$@
	@mv tmp.$@.NEW $@

# This unittest makes sure the precision is the same between the online and offline version [BUG 1067].
.PHONY: diff
diff: hard.complete.diff
diff: hard.incomplete.diff
diff: soft.complete.diff
diff: soft.incomplete.diff
%.diff: online.% notonline.%
	diff <(sort $(filter online.%, $+)) <(sort $(filter notonline.%, $+)) -q


phrases-GT-ZN.fr2en.FILT.gz: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time ${FILTER_MODELS} -f canoe.ini < $<; zcat -f $@ | LC_ALL=C \wc; } &> log.$(basename $@)

## Generate filter_models with code previous to april 3rd 2007
phrases-GT-ZN.fr2en.ORIG.GREP.gz: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(CLEAN_PATH)/filter_models -f canoe.ini -suffix .ORIG.GREP < src; zcat -f $@ | LC_ALL=C \wc; } &> log.$(basename $@)

## Generate filter_models with code previous to april 3rd 2007
phrases-GT-ZN.fr2en.ORIG.LIMIT.gz: src canoe.ini phrases-GT-ZN.fr2en.gz
	{ time $(CLEAN_PATH)/filter_models -z -f canoe.ini -limit phrases-GT-ZN.fr2en -suffix .ORIG.LIMIT < src; zcat -f $@ | LC_ALL=C \wc; } &> log.$(basename $@)

###########################################################
# Generating phrase tables with different -ttable-prune-type values
# TODO: Figure out how to check these
all: prune-types
prune-types: prune-types-hard prune-types-soft
prune-types-hard: tm.hard.ttpt-backward-weights tm.hard.ttpt-forward-weights tm.hard.ttpt-combined tm.hard.ttpt-full
prune-types-soft: tm.soft.ttpt-backward-weights tm.soft.ttpt-forward-weights tm.soft.ttpt-combined tm.soft.ttpt-full

tm.hard.ttpt-%: canoe.ini.ttpt-% src phrases-GT-ZN.fr2en.gz
	{ time ${FILTER_MODELS} -c -f $< -suffix .NEW -tm-online -tm-hard-limit tmp.$@ < src; LC_ALL=C wc tmp.$@.NEW; } >& log.$@
	mv tmp.$@.NEW $@

tm.soft.ttpt-%: canoe.ini.ttpt-% src phrases-GT-ZN.fr2en.gz
	{ time ${FILTER_MODELS} -c -f $< -suffix .NEW -tm-online -tm-soft-limit tmp.$@ < src; LC_ALL=C wc tmp.$@.NEW; } >& log.$@
	mv tmp.$@.NEW $@


############################################################
## CHECKING
WC: $(FILES)
	LC_ALL=C wc -l $^

check1: WC
	diff-round.pl -q -sort -prec 5 notonline.hard.complete   online.hard.complete
check2: WC
	diff-round.pl -q -sort -prec 5 notonline.hard.incomplete online.hard.incomplete
check3: WC
	diff-round.pl -q -sort -prec 5 notonline.soft.complete   online.soft.complete
check4: WC
	diff-round.pl -q -sort -prec 5 notonline.soft.incomplete online.soft.incomplete
check: trans check1 check2 check3 check4


############################################################
## EXTRACT SOURCE PHRASES
%.src: %
	export LC_ALL=C; cut -f1 -d '|' $< | sort > $@

SRC_PHRASE: $(foreach f, $(FILES), $f.src)



############################################################
## MAKING TRANSLATION
trans.%: %  canoe.ini_ src
	canoe -f canoe.ini_ -ttable-multi-prob $< < src > $@ 2> log.canoe.$@

trans.src: phrases-GT-ZN.fr2en.gz src canoe.ini_
	canoe -f canoe.ini_ -ttable-multi-prob $< < src > $@ 2> log.canoe.$@

%.diff:	trans.% trans.src
	@diff -sq trans.src $<

trans: $(foreach f, $(FILES), $f.diff)
	#-@zdiff -sq phrases-GT-ZN.fr2en.FILT.gz phrases-GT-ZN.fr2en.ORIG.GREP.gz
	#-@zdiff -sq phrases-GT-ZN.fr2en.ORIG.LIMIT.gz notonline.soft.incomplete
	#@-export LC_ALL=C; \
	#  zcat phrases-GT-ZN.fr2en.ORIG.LIMIT.gz | sort > tmp; \
	#  cat online.soft.incomplete | sort | diff -sq - tmp; \
	#  rm tmp;


############################################################
## SMALL TTABLE FITLERING
small: phrases.small.soft.FILT phrases.small.soft.fwd.FILT \
       phrases.small.soft.comb.FILT phrases.small.soft.full.FILT \
       phrases.small.hard.FILT phrases.small.hard.fwd.FILT \
       phrases.small.hard.comb.FILT phrases.small.hard.full.FILT
	diff-round.pl -q -sort phrases.small.soft.fwd.FILT phrases.small.soft.FILT

phrases.small.soft.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type backward-weights -tm-soft-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.soft.fwd.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type forward-weights -tm-soft-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.soft.comb.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type combined -tm-soft-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.soft.full.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type full -tm-soft-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.hard.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -tm-hard-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.hard.fwd.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type forward-weights -tm-hard-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.hard.comb.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type combined -tm-hard-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

phrases.small.hard.full.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -c -f canoe.ini.small -ttable-prune-type full -tm-hard-limit $* -no-src-grep
	diff-round.pl -q -sort $@ ref/$@

canoe.ini.small:
	env echo -e "[ttable-limit] 2\n\
[ttable-multi-prob] phrases.small.fr2en\n\
[lmodel-file]\n\
    ${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm.binlm\n\
[ttable-prune-type] backward-weights\n\
[distortion-limit] 0\n\
[ftm] 0:1" > $@

phrases.small.fr2en:
	@env echo -e "ceci ||| a ||| 0.1 0.1 0.1 0.1\n\
ceci ||| b ||| 0.1 0.1 0.01 1e-10\n\
ceci ||| c ||| 0.1 0.1 0.001 0.001\n\
ceci ||| d ||| 0.1 0.1 1e-10 0.01\n\
ceci ||| e ||| 0.1 0.1 0.003 1e-9\n\
ceci ||| f ||| 0.1 0.1 0.01 1e-10\n\
ceci ||| g ||| 0.1 0.1 0.001 0.001\n\
ceci ||| h ||| 0.1 0.1 1e-10 0.01\n\
ceci ||| i ||| 0.1 0.1 0.003 1e-9\n\
cela ||| a ||| 0.1 0.1 0.1 0.1\n\
cela ||| f ||| 0.1 0.1 0.01 1e-10\n\
cela ||| g ||| 0.1 0.1 0.001 0.001\n\
cela ||| h ||| 0.1 0.1 1e-10 0.01\n\
cela ||| i ||| 0.1 0.1 0.003 1e-9\n\
cela ||| b ||| 0.1 0.1 0.01 1e-10\n\
cela ||| c ||| 0.1 0.1 0.001 0.001\n\
cela ||| d ||| 0.1 0.1 1e-10 0.01\n\
cela ||| e ||| 0.1 0.1 0.003 1e-9\n\
encore ||| a ||| 0.1 0.1 0.1 0.1\n\
encore ||| f ||| 0.01 1e-10 0.1 0.1\n\
encore ||| g ||| 0.001 0.001 0.1 0.1\n\
encore ||| h ||| 1e-10 0.01 0.1 0.1\n\
encore ||| i ||| 0.003 1e-9 0.1 0.1\n\
encore ||| f2 ||| 0.01 1e-10 0.1 0.1\n\
encore ||| g2 ||| 0.001 0.001 0.1 0.1\n\
encore ||| h2 ||| 1e-10 0.01 0.1 0.1\n\
encore ||| i2 ||| 0.003 1e-9 0.1 0.1\n\
encore ||| b ||| 0.01 1e-10 0.1 0.1\n\
encore ||| c ||| 0.001 0.001 0.1 0.1\n\
encore ||| d ||| 1e-10 0.01 0.1 0.1\n\
encore ||| e ||| 0.003 1e-9 0.1 0.1\n\
encore ||| b2 ||| 0.01 1e-10 0.1 0.1\n\
encore ||| c2 ||| 0.001 0.001 0.1 0.1\n\
encore ||| d2 ||| 1e-10 0.01 0.1 0.1\n\
encore ||| e2 ||| 0.003 1e-9 0.1 0.1\n\
both ||| a ||| 0.1 0.1 0.1 0.1\n\
both ||| f ||| 0.01 1e-10 0.01 1e-10\n\
both ||| g ||| 0.001 0.001 0.001 0.001\n\
both ||| h ||| 1e-10 0.01 1e-10 0.01\n\
both ||| i ||| 0.003 1e-9 0.003 1e-9\n\
both ||| f2 ||| 0.01 1e-10 0.01 1e-10\n\
both ||| g2 ||| 0.001 0.001 0.001 0.001\n\
both ||| h2 ||| 1e-10 0.01 1e-10 0.01\n\
both ||| i2 ||| 0.003 1e-9 0.003 1e-9\n\
both ||| b ||| 0.01 1e-10 0.01 1e-10\n\
both ||| c ||| 0.001 0.001 0.001 0.001\n\
both ||| d ||| 1e-10 0.01 1e-10 0.01\n\
both ||| e ||| 0.003 1e-9 0.003 1e-9\n\
both ||| b2 ||| 0.01 1e-10 0.01 1e-10\n\
both ||| c2 ||| 0.001 0.001 0.001 0.001\n\
both ||| d2 ||| 1e-10 0.01 1e-10 0.01\n\
both ||| e2 ||| 0.003 1e-9 0.003 1e-9" > $@




canoe.ini.small.binlm:
	env echo -e "[ttable-limit] 2\n\
[ttable-multi-prob] phrases.small.fr2en\n\
[lmodel-file]\n\
    ${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm.binlm#2\n\
[ttable-prune-type] backward-weights\n\
[distortion-limit] 0\n\
[use-ftm]" > $@

all: filter.binlm
# This should fail with binlm
filter.binlm:  canoe.ini.small.binlm src phrases.small.fr2en
	${FILTER_MODELS} -f $< -lm < src 2>&1 | egrep 'Warning: Cannot filter Language Models of binlm type (.*)!  Skipping...' -q


canoe.ini.small.lm:
	env echo -e "[ttable-limit] 2\n\
[ttable-multi-prob] phrases.small.fr2en\n\
[lmodel-file]\n\
    ${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm\n\
[lmodel-order] 2\n\
[ttable-prune-type] backward-weights\n\
[distortion-limit] 0\n\
[use-ftm]" > $@

all: europarl.en.srilm.FILT
# NOTE: you cannot use egrep in quiet mode or else it closes the pipe and
# filter_models gets killed before it gets the chance to write its output.
europarl.en.srilm.FILT: canoe.ini.small.lm src FORCE
	{ ${FILTER_MODELS} -f $< -lm < src; } 2>&1 | egrep 'Warning: Treating order 3 LM .* as order 2'
	[[ -s "$@" ]] || ! echo "No filtered language model was produced" >&2
	! { grep 'ngram 3=' $@; }


.PHONY: old_switch
old_switch:  old_switch.phrases.small.soft.FILT
old_switch:  old_switch.phrases.small.hard.FILT
old_switch:  old_switch.europarl.en.srilm.FILT
old_switch:  old_switch.phrases.small.soft.tm-prune.FILT

all:  old_switch

old_switch.phrases.small.soft.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -f canoe.ini.small -soft-limit $* -no-src-grep 2>&1 \
	| grep 'Error:.*soft-limit' --quiet

old_switch.phrases.small.hard.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -f canoe.ini.small -hard-limit $* -no-src-grep 2>&1 \
	| grep 'Filter models used by' --quiet

old_switch.europarl.en.srilm.FILT:  canoe.ini.small.lm src FORCE
	${FILTER_MODELS} -f $< -L < src 2>&1 | grep 'Error:.*L' --quiet

old_switch.phrases.small.soft.tm-prune.FILT:  %.FILT:  FORCE canoe.ini.small phrases.small.fr2en
	${FILTER_MODELS} -f canoe.ini.small -T linear#5 -tm-soft-limit $* -no-src-grep 2>&1 \
	| grep 'Error:.*T' --quiet


prune-type-full:
	${MAKE} prune-full T=0 L=30
	${MAKE} prune-full T=0 L=10
	${MAKE} prune-full T=0 L=2
	${MAKE} prune-full T=0 L=0
	${MAKE} prune-full T=1e-3 L=2
	${MAKE} prune-full T=1e-3 L=0
	${MAKE} prune-full T=1e-6 L=2
	${MAKE} prune-full T=1e-6 L=1
	${MAKE} prune-full T=1e-6 L=0
	${MAKE} prune-full T=1e-12 L=10
	${MAKE} prune-full T=1e-12 L=3
	${MAKE} prune-full T=1e-12 L=0

L?=0
T?=0
prune-full.T$T-L$L: canoe.ini.small phrases.small.fr2en
	echo ceci cela encore oov | \
	   canoe -f canoe.ini.small -ttable-prune-type full -v 4 \
	   -ttable-limit $L -ttable-threshold $T >& $@

check_prune-full.%: prune-full.%
	egrep '(candidate phrases|Keeping|Discarding|top candidate)' $< | diff - ref/$<

prune-full: check_prune-full.T$T-L$L

############################################################
## (H)LDM filtering
test-dm: phrases.small.with-ldm.FILT phrases.small.with-tpldm.FILT

phrases.small.with-ldm.FILT:  %.FILT:  FORCE canoe.ini.small.dm phrases.small.fr2en dm.small
	echo ceci cela encore oov | \
	   ${FILTER_MODELS} -f canoe.ini.small.dm -ttable-prune-type backward-weights -tm-soft-limit $* -ldm
	diff-round.pl $@ ref/$@
	diff-round.pl dm.small.FILT ref/dm.small.FILT
	diff-round.pl dm.small.FILT.bkoff ref/dm.small.FILT.bkoff

dm.small: phrases.small.fr2en dm.small.bkoff
	cat $< | sed 's/[^|]*$$/ 1 0 0 1 0 0/' > $@

dm.small.bkoff:
	echo 0.85 0.03 0.12 0.89 0.03 0.08 > $@

canoe.ini.small.dm: canoe.ini.small dm.small
	{ cat canoe.ini.small; \
	  echo '[lex-dist-model-file] dm.small'; \
	  echo '[distortion-model] WordDisplacement back-hlex fwd-hlex'; } > $@

phrases.small.with-tpldm.FILT:  %.FILT:  FORCE canoe.ini.small.tpdm phrases.small.fr2en dm.small.tpldm
	echo ceci cela encore oov | \
	   ${FILTER_MODELS} -f canoe.ini.small.tpdm -ttable-prune-type backward-weights -tm-soft-limit $* -ldm
	diff-round.pl $@ ref/$@

canoe.ini.small.tpdm: canoe.ini.small dm.small.tpldm
	{ cat canoe.ini.small; \
	  echo '[lex-dist-model-file] dm.small.tpldm'; \
	  echo '[distortion-model] WordDisplacement back-hlex fwd-hlex'; } > $@

%.tpldm: %
	textldm2tpldm.sh $< 2> log.$@
