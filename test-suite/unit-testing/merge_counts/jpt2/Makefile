#!/ust/bin/make -f
# vim:noet:ts=3
#
# @file Makefile run this test suite
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

SHELL = bash
MERGE_COUNTS ?= merge_counts

.PHONY: all
all: sorted_inputs tmtext_sorted_inputs
	@echo All tests PASSED

.PHONY: clean
clean:
	# Nothing to do


########################################
# merge_counts on sorted inputs
.PHONY: sorted_inputs
sorted_inputs: order_sorted order_src_phrase_sorted order_phrases_sorted

# If merge_counts's inputs are LC_ALL=C sorted, so will its output.
order_sorted:
	echo "Make sure merge_counts's output is LC_ALL=C sorted."
	set -o pipefail; \
	${MERGE_COUNTS} - <(LC_ALL=C sort jpt.hmm3.domain.fr-en.head3000) <(LC_ALL=C sort jpt.ibm2.domain.fr-en.head300) \
	| LC_ALL=C sort -c

# merge_counts's output should have all of its source phrases contiguous.
order_src_phrase_sorted:
	echo "Make sure merge_counts's output source phrases are contiguous."
	[[ `${MERGE_COUNTS} - <(LC_ALL=C sort jpt.hmm3.domain.fr-en.head3000) <(LC_ALL=C sort jpt.ibm2.domain.fr-en.head300) \
	| perl -ple 's/ \|\|\| .*//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc \
	| \wc -l` == 0 ]]

# Not sure this test adds any value compared to order_sorted.
order_phrases_sorted:
	[[ `${MERGE_COUNTS} - <(LC_ALL=C sort jpt.hmm3.domain.fr-en.head3000) <(LC_ALL=C sort jpt.ibm2.domain.fr-en.head300) \
	| perl -ple 's/ [0-9]+$$//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc \
	| \wc -l` == 0 ]]



########################################
# merge_counts on tmtext_sorted inputs
# merge_counts will not produce valid jpt when the inputs have been tmtext_sorted.
.PHONY: tmtext_sorted_inputs
tmtext_sorted_inputs: order_tmtext_sorted shuffled_inputs

# merge_counts no longer fails if its inputs are tmtext_sorted.
order_tmtext_sorted:
	echo "merge_counts no longer fails if its inputs are tmtext_sorted."
	${MERGE_COUNTS} - <(tmtext_sort.sh -1st jpt.hmm3.domain.fr-en.head3000) <(tmtext_sort.sh -1st jpt.ibm2.domain.fr-en.head300) >& /dev/null

# Obviously, shuffled inputs should also be caught by merge_counts as an error.
shuffled_inputs:
	echo "Obviously, shuffled inputs should also be caught by merge_counts as an error."
	! ${MERGE_COUNTS} - <(sort jpt.hmm3.domain.fr-en.head3000) <(sort -r jpt.ibm2.domain.fr-en.head300) >& /dev/null

# We should be able to find example of non-contiguous source phrases in
# merge_counts's output if its inputs are tmtext_sorted.
order_src_phrase_tmtext_sorted:
	[[ `${MERGE_COUNTS} - <(tmtext_sort.sh -1st jpt.hmm3.domain.fr-en.head3000) <(tmtext_sort.sh -1st jpt.ibm2.domain.fr-en.head300) \
	| perl -ple 's/ \|\|\| .*//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc \
	| \wc -l` != 0 ]]

# Not sure this test adds any value compared to order_tmtext_sorted.
order_phrases_tmtext_sorted:
	[[ `${MERGE_COUNTS} - <(tmtext_sort.sh -1st jpt.hmm3.domain.fr-en.head3000) <(tmtext_sort.sh -1st jpt.ibm2.domain.fr-en.head300) \
	| perl -ple 's/ [0-9]+$$//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc \
	| \wc -l` != 0 ]]



################################################################################
# HELPERS
########################################
# merge_counts on shuffled inputs
.PHONY: random_inputs
random_inputs: order_random order_src_phrase_random order_phrases_random
order_random:
	${MERGE_COUNTS} - jpt.hmm3.domain.fr-en.head3000 jpt.ibm2.domain.fr-en.head300 \
	| LC_ALL=C sort -c

order_src_phrase_random:
	[[ `${MERGE_COUNTS} - jpt.hmm3.domain.fr-en.head3000 jpt.ibm2.domain.fr-en.head300 \
	| perl -ple 's/ \|\|\| .*//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc \
	| \wc -l` == 0 ]]

order_phrases_random:
	[[ `${MERGE_COUNTS} - jpt.hmm3.domain.fr-en.head3000 jpt.ibm2.domain.fr-en.head300 \
	| perl -ple 's/ [0-9]+$$//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc \
	| \wc -l` == 0 ]]


order:
	${MERGE_COUNTS} - jpt.hmm3.domain.fr-en.head3000 jpt.ibm2.domain.fr-en.head300 \
	| perl -ple 's/ \|\|\| .*//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc

order2:
	${MERGE_COUNTS} - jpt.hmm3.domain.fr-en.head3000 jpt.ibm2.domain.fr-en.head300 \
	| perl -ple 's/ [0-9]+$$//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc

order_small:
	${MERGE_COUNTS} - <(head -31 jpt.hmm3.domain.fr-en.head3000) <(head -13 jpt.ibm2.domain.fr-en.head300) \
	| perl -ple 's/ \|\|\| .*//' \
	| LC_ALL=C uniq \
	| LC_ALL=C sort \
	| LC_ALL=C uniq -dc

