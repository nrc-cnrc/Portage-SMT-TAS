#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

HEAD := head -n 1000
PPROF := pprof --text

-include Makefile.params

.DEFAULT_GOAL := all
.PHONY: all
all:  prof


.PHONY: clean
clean:


.PHONY:  prof
prof:  PTGSH-244.MMmap/canoe.prof
prof:  PTGSH-244/canoe.prof
prof:  ref/canoe.prof

PTGSH-244.MMmap/canoe.prof:  %/canoe.prof:  ec.test1_en.rule
	${HEAD} $< \
	| CPUPROFILE=$@ ~/sandboxes/PORTAGEshared/src/canoe/canoe -f canoe.ini.cow.MMmap > $*/out 2> $*/log.canoe
	${PPROF} ~/sandboxes/PORTAGEshared/src/canoe/canoe $@ > ${@:.prof=.text}

PTGSH-244/canoe.prof:  %/canoe.prof:  ec.test1_en.rule
	${HEAD} $< \
	| CPUPROFILE=$@ ~/sandboxes/PORTAGEshared/src/canoe/canoe -f canoe.ini.cow > $*/out 2> $*/log.canoe
	${PPROF} ~/sandboxes/PORTAGEshared/src/canoe/canoe $@ > ${@:.prof=.text}

# Build from PORTAGEshared's 1978b440957d7221c14ec56f046e597eb832e8bb.
ref/canoe.prof:  %/canoe.prof:  ec.test1_en.rule
	${HEAD} $< \
	| CPUPROFILE=$@ ~/sandboxes/PORTAGEshared/src/canoe/canoe.ref.PTGSH-244 -f canoe.ini.cow > $*/out 2> $*/log.canoe
	${PPROF} ~/sandboxes/PORTAGEshared/src/canoe/canoe.ref.PTGSH-244 $@ > ${@:.prof=.text}

