#!/usr/bin/make -f
# vim:noet:list

# Makefile - Test the training of HMM word alignment models using MAP
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

CORPUS=hans1_en.lc.gz hans1_fr.lc.gz

all: \
   cmp_map.fr_given_en \
   cmp_map-cat.fr_given_en \
   cmp_map-sym.fr_given_en \
   cmp_map-sym-cat.fr_given_en

cmp_%.fr_given_en: %.fr_given_en
	@$(MAKE) --no-print-directory do_cmp_$*.fr_given_en{,.dist}
	@if [[ -f ref/$*.en_given_fr.gz ]]; then \
	   $(MAKE) --no-print-directory do_cmp_$*.en_given_fr{,.dist} ; \
	fi

do_cmp_%:
	diff-round.pl -p 5 -sort $* ref/$*.gz -q

GLOBAL_OPTIONS=-n1 3 -n2 3 -p 1e-3 -p2 1e-4

ifdef PBS_JOBID
   # When running in a job, use run-parallel.sh to submit jobs so that we can
   # use make -j.  Note that the last target, map-sym-cat.fr_given_en, does not
   # use RUN_SHELL, so that the master still runs one of the actual jobs.
   RUN_SHELL=run-parallel.sh -c
endif

map.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist
	${RUN_SHELL} train_ibm ${GLOBAL_OPTIONS} -v -mimic he-lex $@ $+ >& log.$@

map-cat.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist
	${RUN_SHELL} cat.sh ${GLOBAL_OPTIONS} -v -mimic he-lex $@ $+ >& log.$@

map-sym.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist $(subst fr_given_en,en_given_fr,$@){,.dist}
	${RUN_SHELL} train_ibm ${GLOBAL_OPTIONS} -v -symmetrized liang -mimic he-lex $@ $+ >& log.$@

map-sym-cat.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist $(subst fr_given_en,en_given_fr,$@){,.dist}
	cat.sh ${GLOBAL_OPTIONS} -v -symmetrized liang -mimic he-lex $@ $+ >& log.$@

TEMP_FILES=log.map* run-parallel-logs*
TEMP_DIRS=map*_given_* run-p.*
include ../Makefile.incl
