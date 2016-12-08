#!/usr/bin/make -f
# vim:ts=3:noet
# @file Makefile Check the histograms when filtering phrase tables
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

# Stabilize the output
export PORTAGE_INTERNAL_CALL=1

all: all-multi

all-multi: multi.probs.no-src-grep.gz multi.probs.src.gz
	diff --ignore-matching 'Her Majesty' --ignore-matching 'one in' log.multi.probs.no-src-grep log.multi.probs.no-src-grep.ref
	diff --ignore-matching 'Her Majesty' --ignore-matching 'one in' log.multi.probs.src log.multi.probs.src.ref

TEMP_FILES=dummy.lm canoe.ini* \
           multi.probs.no-src-grep.gz multi.probs.src.gz \
           log.multi.probs.no-src-grep log.multi.probs.src
include ../../Makefile.incl

dummy.lm:
	touch $@

canoe.ini: dummy.lm
	 echo "[ttable-multi-prob]" > $@
	 echo "pt" >> $@
	 echo "[lmodel-file]" >> $@
	 echo "dummy.lm" >> $@
	 echo "[weight-d] 1" >> $@
	 echo "[weight-w] 0" >> $@
	 echo "[weight-l] 1" >> $@
	 echo "[weight-t] 1:1" >> $@
	 echo "[ttable-limit] 30" >> $@
	 echo "[ttable-threshold] 0" >> $@
	 echo "[regular-stack] 300" >> $@
	 echo "[beam-threshold] 0.0001" >> $@
	 echo "[distortion-limit] 7" >> $@
	 echo "[bypass-marked]" >> $@


multi.probs.no-src-grep.gz: canoe.ini
	rm $@ 2> /dev/null || true
	filter_models \
		-no-src-grep \
		-v -z -r \
		-f canoe.ini \
		-suffix .no-src-grep \
		-tm-soft-limit multi.probs \
		>& log.$(basename $@)


multi.probs.src.gz: canoe.ini
	rm $@ 2> /dev/null || true
	filter_models \
		-v -z -r \
		-f canoe.ini \
		-suffix .src \
		-tm-soft-limit multi.probs \
		< src \
		>& log.$(basename $@)
