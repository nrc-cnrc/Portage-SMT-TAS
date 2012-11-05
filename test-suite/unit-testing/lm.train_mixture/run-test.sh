#!/bin/bash
# run-test.sh - Run this test suite, with a non-zero exit status if it fails
#
# PROGRAMMER: George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

# original SRILM version: wts won't match exactly
# train-lm-mixture data/models.lm data/dev_en1.lc > wts

# replacement, with text lms: weights should match
# train_lm_mixture data/models.lm data/dev_en1.lc > wts

# tested version, with binlms: weights should match
train_lm_mixture data/models.binlm data/dev_en1.lc > wts

# compare with ref weights
diff-round.pl -prec 4 data/wts wts

make gitignore
