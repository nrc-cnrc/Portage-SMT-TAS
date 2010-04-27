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

all: multi.probs.no-src-grep.gz multi.probs.src.gz
	diff -I filter_models -I portage_info -I ".one in " log.multi.probs.no-src-grep log.multi.probs.no-src-grep.ref
	diff -I filter_models -I portage_info -I ".one in " log.multi.probs.src log.multi.probs.src.ref

clean:
	-$(RM) dummy.lm canoe.ini*
	-$(RM) multi.probs.no-src-grep.gz multi.probs.src.gz
	-$(RM) log.multi.probs.no-src-grep log.multi.probs.src
	

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
	 echo "[stack] 300" >> $@
	 echo "[beam-threshold] 0.0001" >> $@
	 echo "[distortion-limit] 7" >> $@
	 echo "[bypass-marked]" >> $@


multi.probs.no-src-grep.gz: canoe.ini
	filter_models \
		-no-src-grep \
		-z -r \
		-f canoe.ini \
		-suffix .no-src-grep \
		-soft-limit multi.probs \
		>& log.$(basename $@)


multi.probs.src.gz: canoe.ini
	filter_models \
		-z -r \
		-f canoe.ini \
		-suffix .src \
		-soft-limit multi.probs \
		< src \
		>& log.$(basename $@)
