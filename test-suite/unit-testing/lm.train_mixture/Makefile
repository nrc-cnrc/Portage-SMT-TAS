# Makefile - run this test suite
#
# PROGRAMMER: George Foster / Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009-2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009-2012, Her Majesty in Right of Canada

all: diff_wts
all: diff_lm.mixlm
all: diff_mix.out
all: diff_sentlevel.out

# original SRILM version: wts won't match exactly
srilm-wts: data/models.binlm data/dev_en1.lc
	train-lm-mixture $+ > $@

# replacement, with text lms: weights should match
textlm-wts: data/models.lm data/dev_en1.lc
	train_lm_mixture $+ > $@

# tested version, with binlms: weights should match
wts: data/models.binlm data/dev_en1.lc
	train_lm_mixture $+ > $@


lm.mixlm: data/models.binlm data/dev_en1.lc
	mx-calc-distances.sh em $+ > wts2
	mx-mix-models.sh mixlm wts2 $+ > $@

mix.out: lm.mixlm data/dev_en1.lc
	lm_eval $+ > $@

sentlevel.out: data/sentlevel.mixlm data/dev_en1.lc
	lm_eval $+ > $@


diff_%: %
	diff-round.pl -prec 4 ref/$* $*

TEMP_FILES=wts wts2 lm.mixlm mix.out sentlevel.out
include ../Makefile.incl
