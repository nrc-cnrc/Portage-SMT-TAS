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
	@echo All tests PASSED

cmp_%.fr_given_en: %.fr_given_en
	@$(MAKE) --no-print-directory do_cmp_$*.fr_given_en{,.dist}
	@if [[ -f ref/$*.en_given_fr.gz ]]; then \
	   $(MAKE) --no-print-directory do_cmp_$*.en_given_fr{,.dist} ; \
	fi

do_cmp_%:
	diff-round.pl -p 5 -sort $* ref/$*.gz -q

map.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist
	train_ibm -filter-singletons 0 -v -mimic he-lex $@ $+ >& log.$@

map-cat.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist
	cat.sh -filter-singletons 0 -v -mimic he-lex $@ $+ >& log.$@

map-sym.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist $(subst fr_given_en,en_given_fr,$@){,.dist}
	train_ibm -filter-singletons 0 -v -symmetrized liang -mimic he-lex $@ $+ >& log.$@

map-sym-cat.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist $(subst fr_given_en,en_given_fr,$@){,.dist}
	cat.sh -filter-singletons 0 -v -symmetrized liang -mimic he-lex $@ $+ >& log.$@

TEMP_FILES=log.map* run-parallel-logs*
TEMP_DIRS=map*_given_*
include ../Makefile.incl
