# @file Makefile
# @brief Test suite for combining phrase tables
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

SHELL=/bin/bash

all: test configtool

misc: out.static out.trff.static

allout: $(foreach out,. .trff.,$(foreach type,stati,$(foreach ext,c ch c2,out${out}${type}${ext})))

.SECONDARY:

# We only test the ibm2 cases, because using the hmm cases slows down the test
# suite too much.
CASES_TO_TEST=static tppt-textpt
test: softdiff triangdiff
softdiff: $(foreach c,${CASES_TO_TEST},softdiff_out.$c)
triangdiff: $(foreach c,${CASES_TO_TEST},triangdiff_triang.$c)

diff_%: %
	diff $* ref/$* -q

# softdiff accepts up to 25% of the output lines being different, to allow
# for numerical differences between systems.
softdiff_%: %
	@echo $* diff wc `diff-round.pl $* ref/$* | wc -l` ref wc `wc -l < ref/$*`
	(( `diff-round.pl $* ref/$* | wc -l` * 2 < `wc -l < ref/$*`))

# Note that the ref/triang.* files are for regression testing PTGSH-397:
# replacing our Frankenstein phrase table mess with a base class with a Creator
# factory and a standardized query and merge interface.
triangdiff_%: out.%
	@for x in ref/$**; do \
	   if diff-round.pl -q -sort $* $$x; then \
	      echo $* and $$x match; true; break; \
	   else \
	      false; \
	   fi \
	done


TEMP_FILES=train.en2fr.*.tpa out.* train_*.lm.gz ibm2.* hmm.* jpt.* cpt.*.gz triang.* log.*
TEMP_DIRS=*.tpsa *.tpsa.tmp.* *.tppt
include ../Makefile.incl

#train_%.al:
#	-ln -s ../backward_lm/$@

#ibm2.fr_given_en.gz: train_en.al.bz2 train_fr.al.bz2
#	${RM} $@ `basename $@ .gz`.pos.gz
#	train_ibm -bin -v $@ $+
#
#ibm2.en_given_fr.gz: train_en.al.bz2 train_fr.al.bz2
#	${RM} $@ `basename $@ .gz`.pos.gz
#	train_ibm -bin -rv $@ $+

# Stabilize this test suite by reusing an IBM2 models that is static, from the
# joint2cond test suite
ibm2.%.gz: ../joint2cond/data/ibm2.train.%.gz
	train_ibm -tobin $< $@ >& log.$@

hmm.fr_given_en.gz: train_en.al.bz2 train_fr.al.bz2
	${RM} $@ `basename $@ .gz`.dist.gz
	train_ibm -bin -v -mimic liang $@ $+ >& log.$@

hmm.en_given_fr.gz: train_en.al.bz2 train_fr.al.bz2
	${RM} $@ `basename $@ .gz`.dist.gz
	train_ibm -bin -rv -mimic liang $@ $+ >& log.$@

train_%.lm.gz: train_%.al.bz2
	estimate-ngram -write-lm $@ -text $< >& log.$@

in.10: train_en.al.bz2
	bzcat $< | head -10 > $@

jpt.%.en2fr.gz: %.fr_given_en.gz %.en_given_fr.gz train_en.al.bz2 train_fr.al.bz2
	gen_phrase_tables -write-al all -j $+ | li-sort.sh | gzip > $@

jpt.joint.en2fr.gz: jpt.ibm2.en2fr.gz jpt.hmm.en2fr.gz
	merge_multi_column_counts -top-a $@ $+

cpt.%.en2fr.gz: jpt.%.en2fr.gz ibm2.en_given_fr.gz ibm2.fr_given_en.gz
	joint2cond_phrase_tables -s RFSmoother -s ZNSmoother -multipr fwd \
	   -write-al top \
	   -ibm_l2_given_l1 ibm2.fr_given_en.gz \
	   -ibm_l1_given_l2 ibm2.en_given_fr.gz \
	   -o `basename $@ .en2fr.gz` -z -1 en -2 fr -write-count $<

#cpt.ibm2.en2fr.gz: ibm2.fr_given_en.gz ibm2.en_given_fr.gz train_en.al.bz2 train_fr.al.bz2
#	gen_phrase_tables -s RFSmoother -s ZNSmoother -multipr fwd \
#	   -o cpt.ibm2 -z -1 en -2 fr -write-count \
#	   $+

# This is to create CPTs that will have holes with respect to one another, so
# we can test reading multiple CPTs with some different sources phrases, and
# some different target phrases given a source phrase. We'll be filtering out
# "le" and "les", very frequent words, so that all the types of differences are
# created. Both occur about 2K times in the 50K CPT.
cpt.no-%.en2fr.gz: cpt.ibm2.en2fr.gz
	zcat $< | grep -v ' $* ' | gzip > $@

cpt.%.tppt: cpt.%.gz
	textpt2tppt.sh $< >& log.$@

models.static2: train_fr.lm.gz cpt.joint.en2fr.gz
models.statich: train_fr.lm.gz cpt.hmm.en2fr.gz
models.static: train_fr.lm.gz cpt.ibm2.en2fr.gz

models.tppt-textpt: cpt.no-le.en2fr.gz cpt.no-les.en2fr.tppt

out.trff.%: canoe.ini.% models.%
	canoe -f canoe.ini.$* -input dev1_en.al -trace -ffvals > $@ 2> log.$@

out.triang.%: canoe.ini.% models.%
	canoe -f canoe.ini.$* -input <(head -5 dev1_en.al) -triangularArrayFilename triang.$* > $@ 2> log.$@

out.%: canoe.ini.% models.%
	canoe -f canoe.ini.$* -input dev1_en.al > $@ 2> log.$@

configtool: softdiff
	for cfg in ${CASES_TO_TEST}; do \
	   for cmd in nf nb nt na nt-tppt nd memmap check list-all-files; do \
	      echo -n canoe.ini.$$cfg $$cmd ''; \
	      configtool $$cmd canoe.ini.$$cfg; \
	   done; \
	   echo -n ../canoe.ini.$$cfg ''; \
	   pushd ref >& /dev/null; \
	   configtool check ../canoe.ini.$$cfg; \
	   popd >& /dev/null; \
	done > out.$@
	diff out.$@ ref/out.$@
