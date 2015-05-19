#!/usr/bin/make -f
# vim:noet:ts=3:nowrap

# @file Makefile
# @brief Unit testing gen-jpt-parallel.sh.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada


SHELL = bash

GJP ?= gen-jpt-parallel.sh

.SUFFIXES:
.PHONY: all
all: testsuite

TEMP_FILES=corpus_??? run-parallel-logs-* log.too_many_chunks output.*
TEMP_DIRS=JPTPAR* run-p.[0-9]*
include ../Makefile.incl

.PHONY: testsuite
.PHONY: argument_checking
testsuite: argument_checking



########################################
# You must have an even number of corpora and the must exist.
.PHONY: uneven_corpora
argument_checking: uneven_corpora
uneven_corpora: corpus_en1 corpus_fr1
	${GJP} -o $@ \
		GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
		$+ corpus.uneven 2>&1 \
	| grep -q "You are missing a corpus in your pair of corpora"

.PHONY: non_existing_corpora
argument_checking: non_existing_corpora
non_existing_corpora: corpus_en1 corpus_fr1
	${GJP} -o $@ \
		GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
		$+ corpus_en.bad corpus_fr.bad 2>&1 \
	| egrep -q "Input file corpus_...bad doesn't exist"



########################################
# Making sure gen-jpt-parallel.sh accepts multiple corpora pairs.
.PHONY: multiple_corpora_pairs
testsuite: multiple_corpora_pairs
multiple_corpora_pairs: multiple_corpora_pairs_1 multiple_corpora_pairs_2
multiple_corpora_pairs_1: corpus_en1 corpus_fr1
multiple_corpora_pairs_2: corpus_en1 corpus_fr1 corpus_en2 corpus_fr2
multiple_corpora_pairs_%:
	${GJP} -notreally -o $@ \
	GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
	$+ \
	|| ! echo "Failed to validate several input corpora for $@."



########################################
# Testing gen-jpt-parallel.sh with multiple corpora pairs.
# Checking the validity of the output; 1 pairs vs several pairs.
.PHONY: output_multiple_corpora_pairs
testsuite: output_multiple_corpora_pairs
output_multiple_corpora_pairs: corpus_en1 corpus_fr1 corpus_en2 corpus_fr2 corpus_en3 corpus_fr3
	diff -qs \
	<(${GJP} -o - GPT -a 'IdentityAligner' -j -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null $+ 2> /dev/null) \
	<(${GJP} -o - GPT -a 'IdentityAligner' -j -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null corpus_en corpus_fr 2> /dev/null)



########################################
# Testing the -FILTER option by substituting a for A.
testsuite: filter
.PHONY: filter
filter: filtera
filter: filterthe
filter: filterlonglines

.PHONY: filtera
# There shouldn't be any lowercase a.
filtera: output.filtera
	! grep 'a' $< || echo "Filter failed to substitue a to A." >&2

# Let's replace all lowercase a for A
output.filtera: corpus_en1 corpus_fr1
	${GJP} -o $@ \
		-FILTER "perl -ple 's/a/A/g'" \
		GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
		$+



.PHONY: filterthe
# There shouldn't be any "the".
filterthe: output.filterthe
	! grep 'the' $< || echo "Filter failed to remove occurrences of the." >&2

# Let's replace all lowercase a for A
output.filterthe: corpus_en1 corpus_fr1
	${GJP} -o $@ \
		-FILTER "utf8_casemap -cl | perl -ple 's/.*//g if (/the/)'" \
		GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
		$+ 



.PHONY: filterlonglines
# There shouldn't be any "the".
filterlonglines: output.filterlonglines
	[[ `cat $+ | wc -l` == 1 ]] || echo "Filter failed to remove long lines." >&2
	[[ `grep -c 'EVIDENCE ||| TÉMOIGNAGES ||| 1' $+` == 1 ]] || echo "Filter failed to remove long lines." >&2

# Let's replace all lowercase a for A
output.filterlonglines: corpus_en1 corpus_fr1
	${GJP} -o $@ \
		-FILTER "perl -ple 's/.*//g if (scalar(split(/\s+/)) > 1)'" \
		GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
		$+ 



########################################
# Verify that gen-jpt-parallel.sh will stop with and error if the number of
# chunks exceeds the maximum allowed simultaneous opened files.
testsuite: too_many_chunks
.PHONY: too_many_chunks
too_many_chunks: output.too_many_chunks

output.too_many_chunks: corpus_en1 corpus_fr1
	${GJP} -o $@ \
		-n 50000 \
		-notreally \
		GPT -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 en -2 fr /dev/null /dev/null \
		$+ 2>&1 \
	| tee log.too_many_chunks \
	| egrep 'fatal error: .* doesn.t support merging more than [0-9]+ chunks.'



########################################
# HELPERS
corpus_%1: START=1
corpus_%1: END=56
corpus_%2: START=57
corpus_%2: END=105
corpus_%3: START=106
corpus_%3: END=156

corpus_en%: corpus_en
	lines.py <(seq ${START} ${END}) $< > $@

corpus_fr%: corpus_fr
	lines.py <(seq ${START} ${END}) $< > $@

