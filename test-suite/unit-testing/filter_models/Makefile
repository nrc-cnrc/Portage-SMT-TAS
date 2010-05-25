# @file Makefile Produces all kinds of filtering and make sure they are equivalent.
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

HEAD_SIZE ?= 100
FILES     ?= online.hard.complete online.hard.incomplete online.soft.complete online.soft.incomplete notonline.hard.complete notonline.hard.incomplete notonline.soft.complete notonline.soft.incomplete
CLEAN_PATH ?= ~/sandBoxes/CleanVersion/Portage/src/canoe
PORTAGE ?= /home/portage


.PHONY: clean small check all online online_hard online_soft notonline notonline_hard notonline_soft FORCE
.PRECIOUS: $(foreach f, $(FILES), trans.$f)

all: small check phrases-GT-KN.fr2en.FILT.gz

############################################################
## CLEAN
clean:
	${RM} log.canoe.* log.gen* log.notonline.* log.online.* log.phrases* log.train_ibm.*
	${RM} notonline*
	${RM} online*
	${RM} phrases-GT-KN.fr2en.FILT.gz phrases-GT-KN.fr2en.ORIG.GREP.gz phrases-GT-KN.fr2en.ORIG.LIMIT.gz
	${RM} canoe.ini*
	${RM} phrases.small.*
	${RM} trans.*
	${RM} core.*
	${RM} tmp.online.*

distclean: clean
	${RM} src
	${RM} ibm[12].*
	${RM} phrases-GT-KN.fr2en.gz


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

phrases-GT-KN.fr2en.gz: ibm2.fr_given_en.gz ibm2.en_given_fr.gz
	gen_phrase_tables -v -w1 -m8 -1 fr -2 en -ibm 2 -z \
	  -s GTSmoother -s KNSmoother   \
	  -multipr fwd                  \
	  -o phrases-GT-KN              \
	  ibm2.en_given_fr.gz           \
	  ibm2.fr_given_en.gz           \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.fr.lowercase \
	  ${PORTAGE}/test-suite/regress-small-voc/lc/europarl.fr-en.en.lowercase \
	  >& log.gen_multi_prob_phrase_tables

canoe.ini:
	echo -e "[ttable-multi-prob] \n \
	phrases-GT-KN.fr2en.gz\n \
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

canoe.ini_:
	echo -e "[lmodel-file]\n \
	${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm\n \
	[weight-d] 0.3200787604\n \
	[weight-w] 0.3201824725\n \
	[weight-l] 1\n \
	[weight-t] 0.4:1\n \
	[ttable-limit] 30\n \
	[distortion-limit] 7\n \
	[distortion-model] WordDisplacement\n" > $@

SETUP: phrases-GT-KN.fr2en.gz canoe.ini canoe.ini_


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
FILT_CMD := filter_models -c -f canoe.ini -suffix ".NEW"
online.hard.complete: canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -no-src-grep -tm-online -hard-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@

online.hard.incomplete: src canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -tm-online -hard-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@

online.soft.complete: canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -no-src-grep -tm-online -soft-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@

online.soft.incomplete: src canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -tm-online -soft-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@


notonline.hard.complete: canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -no-src-grep -hard-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@

notonline.hard.incomplete: src canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -hard-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@

notonline.soft.complete: canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -no-src-grep -soft-limit tmp.$@; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@

notonline.soft.incomplete: src canoe.ini phrases-GT-KN.fr2en.gz
	(time $(FILT_CMD) -soft-limit tmp.$@ < $<; LC_ALL=C wc tmp.$@.NEW) &> log.$@
	@mv tmp.$@.NEW $@


phrases-GT-KN.fr2en.FILT.gz: src canoe.ini phrases-GT-KN.fr2en.gz
	(time filter_models -f canoe.ini < $<; LC_ALL=C wc $@) &> log.$(subst .gz,,$@)

## Generate filter_models with code previous to april 3rd 2007
phrases-GT-KN.fr2en.ORIG.GREP.gz: src canoe.ini phrases-GT-KN.fr2en.gz
	(time $(CLEAN_PATH)/filter_models -f canoe.ini -suffix .ORIG.GREP < src; LC_ALL=C wc $@) &> log.$(subst .gz,,$@)

## Generate filter_models with code previous to april 3rd 2007
phrases-GT-KN.fr2en.ORIG.LIMIT.gz: src canoe.ini phrases-GT-KN.fr2en.gz
	(time $(CLEAN_PATH)/filter_models -z -f canoe.ini -limit phrases-GT-KN.fr2en -suffix .ORIG.LIMIT < src; LC_ALL=C wc $@) &> log.$(subst .gz,,$@)


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
trans.%: %  canoe.ini_
	canoe -f canoe.ini_ -ttable-multi-prob $< < src > $@ 2> log.canoe.$@

trans.src: phrases-GT-KN.fr2en.gz src canoe.ini_
	canoe -f canoe.ini_ -ttable-multi-prob $< < src > $@ 2> log.canoe.$@

%.diff:	trans.% trans.src
	@diff -sq trans.src $<

trans: $(foreach f, $(FILES), $f.diff)
	#-@zdiff -sq phrases-GT-KN.fr2en.FILT.gz phrases-GT-KN.fr2en.ORIG.GREP.gz
	#-@zdiff -sq phrases-GT-KN.fr2en.ORIG.LIMIT.gz notonline.soft.incomplete
	#@-export LC_ALL=C; \
	#  zcat phrases-GT-KN.fr2en.ORIG.LIMIT.gz | sort > tmp; \
	#  cat online.soft.incomplete | sort | diff -sq - tmp; \
	#  rm tmp;


############################################################
## SMALL TTABLE FITLERING
small: phrases.small.soft.fwd.FILT phrases.small.soft.FILT \
       phrases.small.hard.fwd.FILT phrases.small.hard.FILT
	diff phrases.small.soft.fwd.FILT phrases.small.soft.FILT

phrases.small.soft.fwd.FILT: FORCE canoe.ini.small.fwd phrases.small.fr2en
	filter_models -f canoe.ini.small.fwd \
	   -soft-limit phrases.small.soft.fwd -no-src-grep
	diff $@ ref/$@

phrases.small.soft.FILT: FORCE canoe.ini.small phrases.small.fr2en
	filter_models -f canoe.ini.small \
	   -soft-limit phrases.small.soft -no-src-grep
	diff $@ ref/$@

phrases.small.hard.fwd.FILT: FORCE canoe.ini.small.fwd phrases.small.fr2en
	filter_models -f canoe.ini.small.fwd \
	   -hard-limit phrases.small.hard.fwd -no-src-grep
	diff $@ ref/$@

phrases.small.hard.FILT: FORCE canoe.ini.small phrases.small.fr2en
	filter_models -f canoe.ini.small \
	   -hard-limit phrases.small.hard -no-src-grep
	diff $@ ref/$@

canoe.ini.small:
	echo -e "[ttable-limit] 2\n\
[ttable-multi-prob] phrases.small.fr2en\n\
[lmodel-file]\n\
    ${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm.binlm\n\
[ttable-prune-type] forward-weights\n\
[distortion-limit] 0\n\
[ftm] 0:1" > $@

canoe.ini.small.fwd:
	echo -e "[ttable-limit] 2\n\
[ttable-multi-prob] phrases.small.fr2en\n\
[lmodel-file]\n\
    ${PORTAGE}/test-suite/regress-small-voc/europarl.en.srilm.binlm\n\
[ttable-prune-type] backward-weights\n\
[distortion-limit] 0" > $@

phrases.small.fr2en:
	echo -e "ceci ||| a ||| 0.1 0.1 0.1 0.1\n\
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
encore ||| e2 ||| 0.003 1e-9 0.1 0.1" > $@

