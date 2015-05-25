# @file Makefile
# @brief Testsuite for walls and zones in canoe
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

TEMP_FILES=out* log.*
include ../Makefile.incl

SHELL=/bin/bash
.SECONDARY:
.SUFFIXES:

CANOE_INI?=canoe.ini

#############################
# Walls
#############################

all: wall
v3: wall-v3
wall: cmp_out-wall-Strict cmp_out-wall-Loose cmp_out-wall-WordStrict
wall-v3: out-v3-wall-Strict out-v3-wall-Loose out-v3-wall-WordStrict

out-wall-%: input-wall ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -distortion-model "Walls#$*" -ffvals -nbest $@:31 > $@ 2> log.$@

out-v3-wall-%: input-wall ${CANOE_INI} lm cpt
	canoe -v 3 -f ${CANOE_INI} -input $< -distortion-model "Walls#$*" -ffvals -nbest $@:31 > $@ 2> log.$@

cmp_out-wall-%: out-wall-%
	diff <(paste out-wall-$*.0000.31best out-wall-$*.0000.31best.ffvals | sort) \
	     <(paste ref/out-wall-$*.0000.31best ref/out-wall-$*.0000.31best.ffvals | sort)
	diff <(paste out-wall-$*.0001.31best out-wall-$*.0001.31best.ffvals | sort) \
	     <(paste ref/out-wall-$*.0001.31best ref/out-wall-$*.0001.31best.ffvals | sort)


#############################
# Hard Walls
#############################

all: hwall
v3: hwall-v3
hwall: cmp_out-hwall-Strict cmp_out-hwall-Loose cmp_out-hwall-WordStrict
hwall-v3: out-v3-hwall-Strict out-v3-hwall-Loose out-v3-hwall-WordStrict

out-hwall-%: input-wall ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -filter-features "Walls#$*" -ffvals -nbest $@:31 > $@ 2> log.$@

out-v3-hwall-%: input-wall ${CANOE_INI} lm cpt
	canoe -v 3 -f ${CANOE_INI} -input $< -filter-features "Walls#$*" -ffvals -nbest $@:31 > $@ 2> log.$@

cmp_out-hwall-%: out-hwall-%
	diff <(paste out-hwall-$*.0000.31best out-hwall-$*.0000.31best.ffvals | sort) \
	     <(paste ref/out-hwall-$*.0000.31best ref/out-hwall-$*.0000.31best.ffvals | sort)
	diff <(paste out-hwall-$*.0001.31best out-hwall-$*.0001.31best.ffvals | sort) \
	     <(paste ref/out-hwall-$*.0001.31best ref/out-hwall-$*.0001.31best.ffvals | sort)


#############################
# Zones
#############################

all: zone
v3: zone-v3
zone: cmp_out-zone-Strict cmp_out-zone-Loose cmp_out-zone-WordStrict
zone-v3: out-v3-zone-Strict out-v3-zone-Loose out-v3-zone-WordStrict

out-zone-%: input-zone ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -distortion-model 'Zones#$*' -ffvals -nbest $@:1000 > $@ 2> log.$@

out-v3-zone-%: input-zone ${CANOE_INI} lm cpt
	canoe -v 3 -f ${CANOE_INI} -input $< -distortion-model 'Zones#$*' -ffvals -nbest $@:1000 > $@ 2> log.$@

cmp_out-zone-%: out-zone-%
	diff <(paste out-zone-$*.0000.1000best out-zone-$*.0000.1000best.ffvals | sort) \
	     <(paste <(zcat ref/out-zone-$*.0000.1000best.gz) <(zcat ref/out-zone-$*.0000.1000best.ffvals.gz) | sort) -q
	diff <(paste out-zone-$*.0001.1000best out-zone-$*.0001.1000best.ffvals | sort) \
	     <(paste <(zcat ref/out-zone-$*.0001.1000best.gz) <(zcat ref/out-zone-$*.0001.1000best.ffvals.gz) | sort) -q


#############################
# Hard Zones
#############################

all: hzone
v3: hzone-v3
hzone: cmp_out-hzone-Strict cmp_out-hzone-Loose cmp_out-hzone-WordStrict
hzone-v3: out-v3-hzone-Strict out-v3-hzone-Loose out-v3-hzone-WordStrict

out-hzone-%: input-zone ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -weight-d 0 -filter-features 'Zones#$*' -ffvals -nbest $@:1000 > $@ 2> log.$@

out-v3-hzone-%: input-zone ${CANOE_INI} lm cpt
	canoe -v 3 -f ${CANOE_INI} -input $< -weight-d 0 -filter-features 'Zones#$*' -ffvals -nbest $@:1000 > $@ 2> log.$@

cmp_out-hzone-%: out-hzone-%
	diff <(paste out-hzone-$*.0000.1000best out-hzone-$*.0000.1000best.ffvals | sort) \
	     <(paste <(zcat ref/out-hzone-$*.0000.1000best.gz) <(zcat ref/out-hzone-$*.0000.1000best.ffvals.gz) | sort) -q
	diff <(paste out-hzone-$*.0001.1000best out-hzone-$*.0001.1000best.ffvals | sort) \
	     <(paste <(zcat ref/out-hzone-$*.0001.1000best.gz) <(zcat ref/out-hzone-$*.0001.1000best.ffvals.gz) | sort) -q



#############################
# Typed walls and zones
#############################

all: typed
typed: cmp_out-typed

out-typed: input-typed ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -ffvals -nbest $@:100 \
	   -distortion-model 'Walls#Strict:Walls#Strict#:Walls#Strict#a:Zones#Strict:Zones#Strict#:Zones#Strict#a' \
	   > $@ 2> log.$@

cmp_out-typed: cmp_out-typed.0000 cmp_out-typed.0001 cmp_out-typed.0002 cmp_out-typed.0003

cmp_out-typed.%: out-typed
	diff <(paste out-typed.$*.100best* | sort)\
	     <(paste ref/out-typed.$*.100best* | sort) -q



#############################
# Local walls
#############################

all: localwalls
localwalls: cmp_out-localwalls

out-localwalls: input-localwalls ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -ffvals -nbest $@:1000 \
	   -distortion-model 'LocalWalls#Strict:LocalWalls#WordStrict:LocalWalls#Loose:LocalWalls#Strict#lw1:Zones#Strict:Walls#Strict' \
	   > $@ 2> log.$@

cmp_out-localwalls: cmp_out-localwalls.0000 cmp_out-localwalls.0001
cmp_out-localwalls.%: out-localwalls
	diff <(paste out-localwalls.$*.1000best* | sort)\
	     <(paste <(zcat ref/out-localwalls.$*.1000best.gz) <(zcat ref/out-localwalls.$*.1000best.ffvals.gz) | sort) -q



##############################################################################
# Long zone - test for interaction between distortion limit and hard zones
##############################################################################

all: longzone
longzone: cmp_out-longzone

out-longzone: input-longzone ${CANOE_INI} lm cpt
	canoe -f ${CANOE_INI} -input $< -v 3 -d 0 -distortion-limit 6 -filter-features 'Zones' \
	   > $@ 2> log.$@


cmp_out-longzone: out-longzone
ifeq ($(CANOE_INI),canoe.ini.cube)
	diff <(egrep '^  (Hyperedge|Discarding) ' log.$< | sort) \
	     <(zcat ref/log.out-longzone.hyperedges.gz | sort) -q
else
	diff <(grep $$'^\tcoverage' log.$< | sort) \
	     <(zcat ref/log.out-longzone.coverage.gz | sort) -q
endif
