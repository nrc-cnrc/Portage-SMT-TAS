# @file Makefile
# @brief Regression test suite for various distortion limit implementations.
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

SHELL=/bin/bash
TEMP_FILES=out-* log.* *.000?.*best*
include ../Makefile.incl

all: go-no_dl go-dl_strict go-swap go-ext go-simple go-itg

go-no_dl: DLOPT=-distortion-limit -1
go-dl_strict: DLOPT=-distortion-limit 3
go-swap: DLOPT=-distortion-limit 3 -dist-phrase-swap
go-ext: DLOPT=-distortion-limit 3 -dist-limit-ext
go-simple: DLOPT=-distortion-limit 3 -dist-limit-simple
go-itg: DLOPT=-distortion-limit 3 -dist-limit-itg

NBEST?=2000

go-%:
	${MAKE} cmp_out-$* CUBE_OPT= -B DLOPT="${DLOPT}"
	${MAKE} cmp_out-$* CUBE_OPT=-cube-pruning -B DLOPT="${DLOPT}"

out-%:
	canoe -f canoe.ini ${DLOPT} ${CUBE_OPT} -input input -nbest nb-$*:${NBEST} > $@ 2> log.$*

cmp_out-%: out-%
	diff <(sort nb-$*.0000.${NBEST}best) <(zcat ref/nb-$*.0000.${NBEST}best.gz | sort) -q
