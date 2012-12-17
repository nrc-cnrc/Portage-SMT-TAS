#!/usr/bin/make -f
# vim:noet:list

# Makefile - Test the symmetric training of HMM word alignment models
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

CORPUS=en.tok fr.tok
SANDBOX=~/sandboxes/PortageTrunk/src/tm
IBM2_PARAMS=-slen 5 -tlen 5 -bksize 5

.PHONY: all
all: cmp

.PHONY: cmp
cmp: cmp_ibm2
cmp: cmp_liang
cmp: cmp_heb
cmp: cmp_ibm1

cmp:
	@echo
	@echo All tests PASSED \(assuming you gave make the -B option\)
	@echo using `which train_ibm`

help:
	@echo PATH=~/sandboxes/PortageTrunk/src/tm:\$$PATH make -B \| egrep --line-buffered -v \'make.*\(ing directory\|aux_cmp\)\'

TEMP_FILES=ibm1.* ibm2.* liang.* heb.* log.* core
include ../Makefile.incl

# Convenient target to run the test using the executable in $(SANDBOX)
sandbox:
	PATH=$(SANDBOX):$$PATH $(MAKE) all -B | egrep --line-buffered -v 'make.*(ing directory|aux_cmp)'

########################################
# HELPERS
cmp_%: %
	diff-round.pl -p 5 -sort $* ref/$* -q

aux_cmp_%:
	diff-round.pl -p 5 -sort $* ref/$* -q


########################################
# IBM1
.PHONY: cmp_ibm1
cmp_ibm1: cmp_ibm1.nonsym.fr_given_en cmp_ibm1.nonsym.en_given_fr 
cmp_ibm1: cmp_ibm1.sym.null.fr_given_en cmp_ibm1.r-sym.null.en_given_fr 
cmp_ibm1: cmp_ibm1.sym.liang.fr_given_en cmp_ibm1.r-sym.liang.en_given_fr 

ibm1.nonsym.fr_given_en: $(CORPUS)
	$(RM) $@
	train_ibm -v -p 0.01 -n2 0 $@ $+ &> log.$@

ibm1.nonsym.en_given_fr: $(CORPUS)
	$(RM) $@
	train_ibm -vr -p 0.01 -n2 0 $@ $+ &> log.$@

ibm1.sym.null.fr_given_en: $(CORPUS)
	$(RM) $@ ibm1.sym.null.en_given_fr
	train_ibm -v -p 0.01 -n2 0 -symmetrized indep $@ $+ &> log.$@
	$(MAKE) aux_cmp_$(subst fr_given_en,en_given_fr,$@)

ibm1.r-sym.null.en_given_fr: $(CORPUS)
	$(RM) $@ ibm1.r-sym.null.fr_given_en
	train_ibm -vr -p 0.01 -n2 0 -symmetrized indep $@ $+ &> log.$@
	$(MAKE) aux_cmp_$(subst en_given_fr,fr_given_en,$@)

ibm1.sym.liang.fr_given_en: $(CORPUS)
	$(RM) $@ ibm1.sym.liang.en_given_fr
	train_ibm -v -p 0.0001 -n2 0 -symmetrized liang $@ $+ &> log.$@
	$(MAKE) aux_cmp_$(subst fr_given_en,en_given_fr,$@)

ibm1.r-sym.liang.en_given_fr: $(CORPUS)
	$(RM) $@ ibm1.r-sym.liang.fr_given_en
	train_ibm -vr -p 0.0001 -n2 0 -symmetrized liang $@ $+ &> log.$@
	$(MAKE) aux_cmp_$(subst en_given_fr,fr_given_en,$@)


########################################
# IBM2
.PHONY: cmp_ibm2
cmp_ibm2: cmp_ibm2.nonsym.fr_given_en cmp_ibm2.nonsym.en_given_fr
cmp_ibm2: cmp_ibm2.sym.null.fr_given_en cmp_ibm2.r-sym.null.en_given_fr
cmp_ibm2: cmp_ibm2.sym.liang.fr_given_en cmp_ibm2.r-sym.liang.en_given_fr

ibm2.nonsym.fr_given_en: $(CORPUS)
	$(RM) $@ $@.pos
	train_ibm -v -p 0.01 $(IBM2_PARAMS) $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.pos

ibm2.nonsym.en_given_fr: $(CORPUS)
	$(RM) $@ $@.pos
	train_ibm -vr -p 0.01 $(IBM2_PARAMS) $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.pos

ibm2.sym.null.fr_given_en: $(CORPUS)
	$(RM) $@ $@.pos ibm2.sym.null.en_given_fr{,.pos}
	train_ibm -v -p 0.01 $(IBM2_PARAMS) -symmetrized indep $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.pos aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).pos

ibm2.r-sym.null.en_given_fr: $(CORPUS)
	$(RM) $@ $@.pos ibm2.r-sym.null.fr_given_en{,.pos}
	train_ibm -vr -p 0.01 $(IBM2_PARAMS) -symmetrized indep $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.pos aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).pos

ibm2.sym.liang.fr_given_en: $(CORPUS)
	$(RM) $@ $@.pos ibm2.sym.liang.en_given_fr{,.pos}
	train_ibm -v -p 0.0001 $(IBM2_PARAMS) -symmetrized liang $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.pos aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).pos

ibm2.r-sym.liang.en_given_fr: $(CORPUS)
	$(RM) $@ $@.pos ibm2.r-sym.liang.fr_given_en{,.pos}
	train_ibm -vr -p 0.0001 $(IBM2_PARAMS) -symmetrized liang $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.pos aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).pos


########################################
# LIANG
.PHONY: cmp_liang
cmp_liang: cmp_liang.nonsym.fr_given_en cmp_liang.nonsym.en_given_fr
cmp_liang: cmp_liang.sym.null.fr_given_en cmp_liang.r-sym.null.en_given_fr
cmp_liang: cmp_liang.sym.liang.fr_given_en cmp_liang.r-sym.liang.en_given_fr
cmp_liang: cmp_liang.sym.liang-var.fr_given_en cmp_liang.r-sym.liang-var.en_given_fr

liang.nonsym.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist
	train_ibm -v -p 0.01 -mimic liang $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.dist

liang.nonsym.en_given_fr: $(CORPUS)
	$(RM) $@ $@.dist
	train_ibm -vr -p 0.01 -mimic liang $@ $+ &> log.$@
	$(MAKE) aux_cmp_$@.dist

liang.sym.null.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist liang.sym.null.en_given_fr{,.dist}
	train_ibm -v -p 0.01 -mimic liang -symmetrized indep $@ $+ >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).dist

liang.r-sym.null.en_given_fr: $(CORPUS)
	$(RM) $@ $@.dist liang.r-sym.null.fr_given_en{,.dist}
	train_ibm -vr -p 0.01 -mimic liang -symmetrized indep $@ $+ >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).dist

liang.sym.liang.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist liang.sym.liang.en_given_fr{,.dist}
	train_ibm -v -n2 4 -p 0 -p2 1e-100 -mimic liang -symmetrized liang $@ $+ >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).dist

liang.r-sym.liang.en_given_fr: $(CORPUS)
	$(RM) $@ $@.dist liang.r-sym.liang.fr_given_en{,.dist}
	train_ibm -vr -n2 4 -p 0 -p2 1e-100 -mimic liang -symmetrized liang $@ $+ >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).dist

liang.sym.liang-var.fr_given_en: $(CORPUS)
	$(RM) $@ $@.dist liang.sym.liang-var.en_given_fr{,.dist}
	train_ibm -v -n2 4 -p 0 -p2 1e-100 -mimic liang -symmetrized liang-variant $@ $+ >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).dist

liang.r-sym.liang-var.en_given_fr: $(CORPUS)
	$(RM) $@ $@.dist liang.r-sym.liang-var.fr_given_en{,.dist}
	train_ibm -vr -n2 4 -p 0 -p2 1e-100 -mimic liang -symmetrized liang-variant $@ $+ >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).dist


########################################
# HE BASELINE
.PHONY: cmp_heb
cmp_heb: cmp_heb.nonsym.fr_given_en cmp_heb.nonsym.en_given_fr
cmp_heb: cmp_heb.sym.null.fr_given_en cmp_heb.r-sym.null.en_given_fr
cmp_heb: cmp_heb.sym.liang.fr_given_en cmp_heb.r-sym.liang.en_given_fr
cmp_heb: cmp_heb.sym.liang-var.fr_given_en cmp_heb.r-sym.liang-var.en_given_fr

heb.nonsym.fr_given_en: $(CORPUS) en.cls
	$(RM) $@ $@.dist
	train_ibm -v -p 0.01 -word-classes-l1 en.cls -mimic he-baseline $@ $(CORPUS) &> log.$@
	$(MAKE) aux_cmp_$@.dist

heb.nonsym.en_given_fr: $(CORPUS) fr.cls
	$(RM) $@ $@.dist
	train_ibm -vr -p 0.01 -word-classes-l1 fr.cls -mimic he-baseline $@ $(CORPUS) &> log.$@
	$(MAKE) aux_cmp_$@.dist

heb.sym.null.fr_given_en: $(CORPUS) en.cls fr.cls
	$(RM) $@ $@.dist heb.sym.null.en_given_fr{,.dist}
	train_ibm -v -p 0.01 -word-classes-l1 en.cls -word-classes-l2 fr.cls -mimic he-baseline -symmetrized indep $@ $(CORPUS) >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).dist

heb.r-sym.null.en_given_fr: $(CORPUS) en.cls fr.cls
	$(RM) $@ $@.dist heb.r-sym.null.fr_given_en{,.dist}
	train_ibm -vr -p 0.01 -word-classes-l1 fr.cls -word-classes-l2 en.cls -mimic he-baseline -symmetrized indep $@ $(CORPUS) >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).dist

heb.sym.liang.fr_given_en: $(CORPUS) en.cls fr.cls
	$(RM) $@ $@.dist heb.sym.liang.en_given_fr{,.dist}
	train_ibm -v -n2 4 -p 0 -p2 1e-100 -word-classes-l1 en.cls -word-classes-l2 fr.cls -mimic he-baseline -symmetrized liang $@ $(CORPUS) >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).dist

heb.r-sym.liang.en_given_fr: $(CORPUS) en.cls fr.cls
	$(RM) $@ $@.dist heb.r-sym.liang.fr_given_en{,.dist}
	train_ibm -vr -n2 4 -p 0 -p2 1e-100 -word-classes-l1 fr.cls -word-classes-l2 en.cls -mimic he-baseline -symmetrized liang $@ $(CORPUS) >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).dist

heb.sym.liang-var.fr_given_en: $(CORPUS) en.cls fr.cls
	$(RM) $@ $@.dist heb.sym.liang-var.en_given_fr{,.dist}
	train_ibm -v -n2 4 -p 0 -p2 1e-100 -word-classes-l1 en.cls -word-classes-l2 fr.cls -mimic he-baseline -symmetrized liang-variant $@ $(CORPUS) >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst fr_given_en,en_given_fr,$@) \
	  aux_cmp_$(subst fr_given_en,en_given_fr,$@).dist

heb.r-sym.liang-var.en_given_fr: $(CORPUS) en.cls fr.cls
	$(RM) $@ $@.dist heb.r-sym.liang-var.fr_given_en{,.dist}
	train_ibm -vr -n2 4 -p 0 -p2 1e-100 -word-classes-l1 fr.cls -word-classes-l2 en.cls -mimic he-baseline -symmetrized liang-variant $@ $(CORPUS) >& log.$@
	$(MAKE) aux_cmp_$@.dist aux_cmp_$(subst en_given_fr,fr_given_en,$@) \
	  aux_cmp_$(subst en_given_fr,fr_given_en,$@).dist

