# Makefile - test all the IBMOchAligner variants
#
# Author: Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada
# Copyright 2012, Her Majesty in Right of Canada


AL_FORMAT?=matrix
all: $(foreach x,-2 -1 1 2 3 4,cmp_ch2en.al$x.${AL_FORMAT})

TEMP_FILES=ch2en.*.${AL_FORMAT}
include ../Makefile.incl

ch2en.al%.${AL_FORMAT}: ch.lc en.lc en-ch.giza ch-en.giza
	align-words -a 'IBMOchAligner $*' -o ${AL_FORMAT} -ibm 0 -giza2 $+ > $@

all: GDF GDFA

GDF: ch2en.syn-GDF.${AL_FORMAT} ch2en.al3.${AL_FORMAT}
	diff $+

GDFA: ch2en.syn-GDFA.${AL_FORMAT} ch2en.al4.${AL_FORMAT}
	diff $+

ch2en.syn-%.${AL_FORMAT}: ch.lc en.lc en-ch.giza ch-en.giza
	align-words -a $* -o ${AL_FORMAT} -ibm 0 -giza2 $+ > $@

cmp_%: %
	diff $* ref/$*
