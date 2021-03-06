# vim:noet:list
# Makefile - Test decoding using sentence-by-sentence canoe-parallel
#
# PROGRAMMER: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

SHELL=bash

all: diff-shuf diff-subset diff-subset-lf cmp-long cli-errors

TEMP_FILES=shuf subset subset-lf log.* long-*
D=daemon-dir
TEMP_DIRS=$D-* canoe-parallel.* run-p.*
include ../Makefile.incl

shuf:
	mkdir -p $D-$@
	seq 0 9 | shuf > $D-$@/jobs
	./launch.sh $D-$@ > $@

subset:
	mkdir -p $D-$@
	echo 2 5 9 | tr ' ' $$'\n' > $D-$@/jobs
	./launch.sh $D-$@ > $@

subset-lf:
	mkdir -p $D-$@
	echo 2 5 9 | tr ' ' $$'\n' > $D-$@/jobs
	./launch.sh $D-$@ -load-first > $@


.PHONY: long
long: long-nopar long-parbaseline long-bysent

cmp-long: long
	cmp long-nopar long-parbaseline
	cmp long-nopar long-bysent

long-nopar: input-long canoe.ini
	time-mem canoe -f canoe.ini -input $< > $@ 2> log.$@

long-parbaseline: input-long canoe.ini
	time-mem canoe-parallel.sh -rp-j 2 -v -v -d -no-lb -n 4 canoe -f canoe.ini < $< > $@ 2> log.$@

long-bysent: input-long canoe.ini
	time-mem canoe-parallel.sh -rp-j 2 -v -v -d -lb-by-sent -n 4 canoe -f canoe.ini < $< > $@ 2> log.$@

diff-%: %
	sort $* | diff ref/$*-sort -

.PHONY: cli-errors
cli-errors: cli-bad-opt cli-bad-value cli-file-not-found cli-bad-canoe-ini

cli-bad-opt:
	! canoe-parallel.sh -rp-j 2 -v -v -d -lb-by-sent -n 4 canoe -f canoe.ini -lm-file foo < input &> log.$@

cli-bad-value:
	! canoe-parallel.sh -rp-j 2 -v -v -d -lb-by-sent -n 4 canoe -f canoe.ini -basdf < input &> log.$@

cli-file-not-found:
	! canoe-parallel.sh -rp-j 2 -v -v -d -lb-by-sent -n 4 canoe -f canoe.ini -lmodel-file /no/such/file < input &> log.$@

cli-bad-canoe-ini:
	! canoe-parallel.sh -rp-j 2 -v -v -d -lb-by-sent -n 4 canoe -f canoe.ini-foo < input &> log.$@
