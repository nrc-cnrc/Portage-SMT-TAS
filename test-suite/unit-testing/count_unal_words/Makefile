# vim:noet:ts=3

# @file Makefile
# @frief Tests count_unal_words
#
# @author Eric Joanis
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

TEMP_FILES=log.* out.*
TEMP_DIRS=
include ../Makefile.incl

SHELL:=/bin/bash

# Make this test suite locale-insensitive
export LC_ALL=C

all: diff.count_all diff.count_sort diff.count_sort_head_cut

count_all:
	time-mem -p 1 count_unal_words -l 2 ../joint2cond/data/jpt.* > out.$@

diff.count_all: count_all
	diff -q <(li-sort.sh < out.$<) <(zcat ref/out.count_sort.gz | li-sort.sh)

count_sort:
	time-mem -p 1 count_unal_words -l 2 ../joint2cond/data/jpt.* | { sort -nr -k2,2; true; } > out.$@

diff.count_sort: count_sort
	diff -q out.$< <(zcat ref/out.count_sort.gz)

count_sort_head_cut:
	time-mem -p 1 count_unal_words -l 2 ../joint2cond/data/jpt.* | { sort -nr -k2,2; true; } | head -50 | cut -d' ' -f1 > out.$@

diff.count_sort_head_cut: count_sort_head_cut
	diff out.$< ref/out.$<
