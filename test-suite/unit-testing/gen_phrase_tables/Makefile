#!/usr/bin/make -f
# vim:noet:list

# Makefile - Test writing word alignments out with the phrase table.
#            also testing writing counts in CPTs.
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

all: gen merge cpt pcpt pcpt_rmem count

TEMP_FILES=log.* ibm2.* jpt.* pjpt.* cpt.* merge[12].* pcpt.* pcpt_rmem.* \
           c-cpt.* ca-cpt.* c-pcpt.* ca-pcpt.* \
           externalAligner.*.gz
TEMP_DIRS=JPTPAR[1-9]* run-p.*
include ../Makefile.incl

.SECONDARY:

.PHONY: gen
gen: cmp_jpt.top cmp_jpt.all cmp_pjpt.all cmp_pjpt.top

IBM2=ibm2.en_given_fr.gz ibm2.fr_given_en.gz
CORPUS=corpus_en corpus_fr

ibm2.en_given_fr.gz:
	${RM} `basename $@ .gz`{.pos,}.gz
	train_ibm -v -bin -filter-singletons 0 ibm2.en_given_fr.gz ${CORPUS} >& log.$@

ibm2.fr_given_en.gz:
	${RM} `basename $@ .gz`{.pos,}.gz
	train_ibm -vr -bin -filter-singletons 0 ibm2.fr_given_en.gz ${CORPUS} >& log.$@

jpt.%: ${IBM2}
	gen_phrase_tables -write-al $* -j -v -1 en -2 fr \
	   ${IBM2} ${CORPUS} 2> log.$@ > $@

pjpt.%: ${IBM2}
	gen-jpt-parallel.sh -n 4 GPT -write-al $* -j -v -1 en -2 fr \
	   ${IBM2} ${CORPUS} 2> log.$@ > $@

cmp_%: %
	zcat ref/$*.gz | diff $* - -q

cmp_dir_%: %
	zcat ref/$*.en2fr.gz | diff-round.pl -p 5 $*.en2fr - -q
	zcat ref/$*.fr2en.gz | diff-round.pl -p 5 $*.fr2en - -q

.PHONY: cpt
cpt: cmp_dir_cpt.all cmp_dir_cpt.top

cpt.%: ${IBM2}
	gen_phrase_tables -write-al $* -multipr both -o $@ -1 en -2 fr -v \
	   ${IBM2} ${CORPUS} 2> log.$@


.PHONY: merge
merge: cmp_merge1.jptpar.all cmp_merge1.jptpar.top cmp_merge2.jptpar.all cmp_merge2.jptpar.top

merge1.%:
	merge_multi_column_counts -a $@ $*/aaa*.jpt.gz 2> log.$@

merge2.%:
	zcat $*/aaa*.jpt.gz | joint2cond_phrase_tables -write-al keep -j 2> log.$@ > $@

merge.%_cmp: merge1.% merge2.%
	diff-round.pl -sort -q $+


.PHONY: pcpt
pcpt: cmp_dir_pcpt.all cmp_dir_pcpt.top

pcpt.%: pjpt.%
	${RM} $@.*
	joint2cond_phrase_tables -write-al $* -multipr both -o $@ -1 en -2 fr -v \
	   $< 2> log.$@

.PHONY: pcpt_rmem
pcpt_rmem: pcpt_rmem.all pcpt_rmem.top

pcpt_rmem.%: pjpt.% pcpt.%
	${RM} $@.*
	joint2cond_phrase_tables -write-al $* -multipr both -o $@ -1 en -2 fr -v \
	   -reduce-mem $< 2> log.$@
	diff-round.pl -sort -q $@.en2fr pcpt.$*.en2fr
	diff-round.pl -sort -q $@.fr2en pcpt.$*.fr2en

.PHONY: count
count: cmp_dir_c-cpt cmp_dir_ca-cpt cmp_dir_c-pcpt cmp_dir_ca-pcpt

c-cpt: ${IBM2}
	gen_phrase_tables -write-count -multipr both -o $@ -1 en -2 fr -v \
	   ${IBM2} ${CORPUS} 2> log.$@

ca-cpt: ${IBM2}
	gen_phrase_tables -write-count -write-al top -multipr both -o $@ -1 en -2 fr -v \
	   ${IBM2} ${CORPUS} 2> log.$@

c-pcpt: pjpt.top
	${RM} $@.*
	joint2cond_phrase_tables -write-count -multipr both -o $@ -1 en -2 fr -v \
	   $< 2> log.$@

ca-pcpt: pjpt.top
	${RM} $@.*
	joint2cond_phrase_tables -write-count -write-al top -multipr both -o $@ -1 en -2 fr -v \
	   $< 2> log.$@


########################################
# external aligner
.PHONY: externalAligner
all:  externalAligner

externalAligner:  SHELL=bash
externalAligner:  externalAligner.X.gz externalAligner.a.gz
	ls -l $+
	cmp <(zcat $(word 1, $+)) <(zcat $(word 2, $+))

externalAligner.X.gz:  corpus_en corpus_fr  ibm2.en2fr.align.gz
	gen_phrase_tables  -j -m 8 -1 en -2 fr -ext $+ | LC_ALL=C sort | gzip > $@

# This is our reference create using the -a switch.
externalAligner.a.gz:  ibm2.en2fr.align.gz  corpus_en corpus_fr
	gen_phrase_tables -ibm 1 -j -m 8 -1 en -2 fr -a "ExternalAligner $(filter %.en2fr.align.gz, $+)" /dev/null /dev/null $(filter corpus%, $+) | LC_ALL=C sort | gzip > $@

ibm2.en2fr.align.gz:  ibm2.fr_given_en.gz ibm2.en_given_fr.gz  corpus_en corpus_fr
	align-words -o sri -ibm 2 $+ | gzip > $@

########################################
# errors due to empty files (or files with differing line counts) should be caught and fatal
.PHONY: empty_files
all: empty_files
empty_files: externalAligner.empty externalAligner.src_empty externalAligner.tgt_empty

externalAligner.empty: corpus_en corpus_fr /dev/null
	! { gen_phrase_tables -v -j -m 8 -1 en -2 fr -ext $+; }
	! { gen-jpt-parallel.sh -n 2 -nw 2 -rp -nocluster GPT -v -j -m 8 -1 en -2 fr -ext $+; }

externalAligner.src_empty:  /dev/null corpus_fr ibm2.en2fr.align.gz
	! { gen_phrase_tables -v -j -m 8 -1 en -2 fr -ext $+; }
	! { gen-jpt-parallel.sh -n 2 -nw 2 -rp -nocluster GPT -v -j -m 8 -1 en -2 fr -ext $+; }

externalAligner.tgt_empty:  corpus_en /dev/null ibm2.en2fr.align.gz
	! { gen_phrase_tables -v -j -m 8 -1 en -2 fr -ext $+; }
	! { gen-jpt-parallel.sh -n 2 -nw 2 -rp -nocluster GPT -v -j -m 8 -1 en -2 fr -ext $+; }
