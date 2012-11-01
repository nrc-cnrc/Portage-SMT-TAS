# Makefile - Test the ibmcat program to output word alignment models in
#            human readable format.
# Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

all: test_ibm1 test_ibm2 test_hmm test_sort
	@echo All tests PASSED \(assuming you ran with -B\)

.SECONDARY:

TEMP_FILES=ibm2.tiny* hmm.tiny*
include ../Makefile.incl

ibm2.tiny: 1line.en 1line.fr
	${RM} $@{,.pos}
	train_ibm -slen 5 -tlen 5 -bksize 5 $@ $^ &> $@.log
	li-sort.sh < $@ > $@.tmp
	mv $@.tmp $@

hmm.tiny: 1line.en 1line.fr
	${RM} $@{,.dist}
	train_ibm -hmm -map-tau 5 $@ $^ &> $@.log

test_ibm1: \
   ibm2.tiny.cmp_ibm1 \
   ibm2.tiny.bin.cmp_ibm1 \
   ibm2.tiny.c.gz.cmp_ibm1 \
   ibm2.tiny.bin.c.gz.cmp_ibm1

%.cmp_ibm1: %
	ibmcat $* | li-sort.sh | diff-round.pl ibm2.tiny - -q

test_ibm2: \
   ibm2.tiny.pos.cmp_ibm2 \
   ibm2.tiny.bin.pos.cmp_ibm2 \
   ibm2.tiny.c.pos.gz.cmp_ibm2 \
   ibm2.tiny.bin.c.pos.gz.cmp_ibm2

%.cmp_ibm2: test_ibm1
	ibmcat $* | diff-round.pl ibm2.tiny.pos - -q

test_hmm: hmm.tiny hmm.tiny.c.gz hmm.tiny.bin hmm.tiny.bin.c.gz \
   hmm.tiny.dist.cmp_hmm \
   hmm.tiny.bin.dist.cmp_hmm \
   hmm.tiny.c.dist.cmp_hmm \
   hmm.tiny.bin.c.dist.cmp_hmm

hmm.tiny.c: hmm.tiny.c.gz
hmm.tiny.bin.c: hmm.tiny.bin.c.gz

%.dist.cmp_hmm: %
	ibmcat $*.dist | diff-round.pl hmm.tiny.dist - -q

test_sort: \
	ibm2.tiny.bin.cmp_sort \
	ibm2.tiny.bin.c.gz.cmp_sort \
	hmm.tiny.bin.cmp_sort \
	hmm.tiny.bin.c.gz.cmp_sort

%.cmp_sort: %
	diff-round.pl -prec 12 -sort "ibmcat $* |" "ibmcat -sort $* |"

%.bin: %
	${RM} $@{,.pos,.dist}
	train_ibm -tobin $* $@

# Compressed files have .c.gz at the end instead of just .gz, because otherwise
# MagicStreams treat <file> and <file>.gz as interchangeable and the tests
# will interfere with each other.
%.bin.c.gz: %
	${RM} $*.bin.c{,.pos,.dist}.gz
	train_ibm -tobin $* $@

%.c.gz: %
	${RM} $*.c{,.pos,.dist}.gz
	train_ibm -frombin $* $@

