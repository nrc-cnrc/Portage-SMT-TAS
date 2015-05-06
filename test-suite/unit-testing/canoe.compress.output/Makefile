#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada


CANOE_INI := canoe.ini.cow
CANOE_INI_DIR := ${PORTAGE}/test-suite/systems/toy-regress-en2fr
MAX := 111

-include Makefile.params

vpath ${CANOE_INI} ${CANOE_INI_DIR}

.DEFAULT_GOAL := all
.PHONY: all
all: run_canoe test
run_canoe:  append
run_canoe:  no_append
run_canoe:  cp


TEMP_FILES=
TEMP_DIRS=*lattice* *best* log.* *.wk canoe-parallel.*
include ../Makefile.incl


# Add compressed extension when appropriate.
%.compressed:  GZ := .gz
# Where wW means with working directory.
1best.append.wW:  WORKDIR := append.uncompressed.wk/
1best.append.wW.compressed:  WORKDIR := append.compressed.wk/
append:  1best.append  1best.append.compressed  1best.append.wW 1best.append.wW.compressed
1best.append  1best.append.compressed  1best.append.wW 1best.append.wW.compressed:  1best.%:  ${CANOE_INI}
	$(and ${WORKDIR}, mkdir -p ${WORKDIR})
	seq 1 ${MAX} \
	| canoe \
		-append \
		-f $< \
		-lattice ${WORKDIR}$*.lattice${GZ} \
		-nbest ${WORKDIR}$*.nbest${GZ}:88 \
		-ffvals \
		-palign \
		> $@ \
		2> log.$@


# Note: defN: use default Nbest size (which is 100)
cp:  1best.cp.88  1best.cp.88.compressed
cp:  1best.cp.100  1best.cp.100.compressed
cp:  1best.cp.defN 1best.cp.defN.compressed
1best.cp.88 1best.cp.88.compressed:  NBEST_SZ:=:88
1best.cp.100 1best.cp.100.compressed:  NBEST_SZ:=:100
1best.cp.defN 1best.cp.defN.compressed:  NBEST_SZ:=
1best.cp.100: WORKDIR := cp.100.uncompressed.wk/
1best.cp.100.compressed: WORKDIR := cp.100.compressed.wk/
1best.cp.defN: WORKDIR := cp.defN.uncompressed.wk/
1best.cp.defN.compressed: WORKDIR := cp.defN.compressed.wk/
1best.cp.88 1best.cp.88.compressed 1best.cp.100 1best.cp.100.compressed 1best.cp.defN 1best.cp.defN.compressed:  1best.%:  ${CANOE_INI}
	$(and ${WORKDIR}, mkdir -p ${WORKDIR})
	seq 1 ${MAX} \
	| canoe-parallel.sh \
		canoe \
			-append \
			-f $< \
			-lattice ${WORKDIR}$*.lattice${GZ} \
			-nbest ${WORKDIR}$*.nbest.${GZ}${NBEST_SZ} \
			-ffvals \
			-palign \
		> $@ \
		2> log.$@


no_append:  1best.no_append.compressed 1best.no_append
1best.no_append:  WORKDIR := no_append.uncompressed.wk/
1best.no_append.compressed:  WORKDIR := no_append.compressed.wk/
1best.no_append  1best.no_append.compressed:  1best.%:  ${CANOE_INI}
	$(and ${WORKDIR}, mkdir -p ${WORKDIR})
	seq 1 ${MAX} \
	| canoe \
		-f $< \
		-lattice ${WORKDIR}$*.lattice${GZ} \
		-nbest ${WORKDIR}$*.nbest${GZ}:88 \
		-ffvals \
		-palign \
		> $@ \
		2> log.$@
	zcat -f ${WORKDIR}$*.nbest.????.88best.ffvals${GZ} $(and ${GZ}, | gzip ) > $*.nbest.88best.ffvals${GZ}
	zcat -f ${WORKDIR}$*.nbest.????.88best.pal${GZ} $(and ${GZ}, | gzip ) > $*.nbest.88best.pal${GZ}
	zcat -f ${WORKDIR}$*.lattice.????${GZ} $(and ${GZ}, | gzip ) > $*.lattice${GZ}
	zcat -f ${WORKDIR}$*.lattice.????.state${GZ} $(and ${GZ}, | gzip ) > $*.lattice.state${GZ}
	zcat -f ${WORKDIR}$*.nbest.????.88best${GZ} $(and ${GZ}, | gzip ) > $*.nbest.88best${GZ}




.PHONY:  test  $(addsuffix _test, 1best nbest ffvals pal lattice lattice_state cp_append)
test:  1best_test
1best_test:  append cp no_append
	[[ `md5sum 1best.*append* 1best.cp.88* | cut -f 1 -d ' ' | sort -u | \wc -l` -eq 1 ]]

test: other_files_test
other_files_test: append cp no_append
	for f in nbest.88best nbest.88best.ffvals nbest.88best.pal lattice lattice.state; do \
	   zd="zdiff -q append.$$f"; \
	   for t in append no_append; do \
	      for x in $$t.$$f $$t.compressed.$$f.gz; do \
	         echo $$zd $$x; $$zd $$x; \
	      done; \
	   done; \
	   g=$${f/88best.}; g=$${g/88/n}; \
	   for x in cp.88.$$g cp.88.compressed.$$g.gz; do \
	      echo $$zd $$x; $$zd $$x; \
	   done; \
	   for x in append.uncompressed.wk/append.wW.$$f append.compressed.wk/append.wW.compressed.$$f.gz; do \
	      echo $$zd $$x; $$zd $$x; \
	   done; \
	done

test:  cp_append_test
cp_append_test: cp
	for t in nbest.nbest nbest.ffvals nbest.pal lattice lattice.state; do \
	   zd="zdiff -q cp.100.uncompressed.wk/cp.100.$$t"; \
	   for n in 100 defN; do \
	      for x in cp.$$n.uncompressed.wk/cp.$$n.$$t cp.$$n.compressed.wk/cp.$$n.compressed.$$t.gz; do \
	         echo $$zd $$x; $$zd $$x; \
	      done; \
	   done; \
	done
