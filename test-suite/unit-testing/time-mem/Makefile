#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Testsuite for time-mem.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada



-include Makefile.params

TEMP_FILES=run-parallel-logs-*
TEMP_DIRS=canoe-parallel.*
include ../Makefile.incl


# We need bash -O compat31 on bash versions that support it, but earlier
# versions crash when one provides that option.  So we set SHELL with or
# without it, depending on whether it fails to start with -O compat31 or not.
SHELL := $(shell if bash -O compat31 -c true 2> /dev/null; \
           then echo bash -O compat31; else echo bash; fi)

.PHONY: all
all: testsuite



########################################
# Defining the testsuite.
.PHONY: testsuite
testsuite: time-mem.bloater
testsuite: error_code
testsuite: detect
testsuite: tally_canoe
testsuite: tally_dirs



########################################
# time-mem should return correctly the return code.
.PHONY: error_code
error_code: error_code_bad
error_code: error_code_good

.PHONY: error_code_bad
error_code_bad:
	! { time-mem false; }

.PHONY: error_code_good
error_code_good:
	time-mem true


########################################
# make sure time-mem detects certain key program.
.PHONY: detect
detect: detect_rat.sh
detect: detect_cat.sh
detect: detect_cow.sh
detect: detect_gen-jpt-parallel.sh
detect: detect_canoe-parallel.sh
detect: detect_invalid

detect_%:
	[[ `time-mem -d $* 2>&1` =~ "Parsing command $*" ]]

detect_invalid: detect_%:
	[[ `LC_ALL=C time-mem -d $* 2>&1` =~ "$*: command not found" ]]



########################################
# For log.canoe.ini.cow we should be tallying on the fly.
# But for log.canoe.ini.cow.tallied we MUSTn't tallying on the fly since it has already been tallied.
tally_canoe:
	[[ `time-mem -T -d  log.canoe.ini.cow 2>&1` =~ "Calculating on the fly" ]]
	time-mem -T log.canoe.ini.cow | diff - log.canoe.ini.cow.ref -q
	time-mem-tally.pl -no-dir log.canoe.ini.cow | diff - log.canoe.ini.cow.time-mem-tally.ref -q
	! [[ `time-mem -T -d log.canoe.ini.cow.tallied 2>&1` =~ "Calculating on the fly" ]]

debug_tally:
	egrep 'Single-|RP-Totals|run_cmd rc=0|Master-Wall-Time' log.canoe.ini.cow
	egrep 'Single-|RP-Totals|run_cmd rc=0' log.canoe.ini.cow | egrep -o '(Real|Wall time) [0-9.]+s' | egrep -o '[0-9.]+' | sum.pl
	egrep 'Master-Wall-Time' log.canoe.ini.cow
	egrep 'Single-|RP-Totals|run_cmd rc=0' log.canoe.ini.cow | egrep -o '(User|Sys|CPU time) [0-9.]+s' | egrep -o '[0-9.]+' | sum.pl
	egrep 'Single-|RP-Totals|run_cmd rc=0' log.canoe.ini.cow | egrep -o '(VMEM) [0-9.]+' | egrep -o '[0-9.]+' | sum.pl -m
	egrep 'Single-|RP-Totals|run_cmd rc=0' log.canoe.ini.cow | egrep -o '(RAM) [0-9.]+' | egrep -o '[0-9.]+' | sum.pl -m
	time-mem -T log.canoe.ini.cow

tally_dirs:
	time-mem-tally.pl test{1,2}/logs/log* | diff - tally.test1_2.ref -q

########################################
# This unittest uses bloater and verify that the time & memory are approximately correctly reported.
# Sometimes the mem checking happens a bit too soon, so test for > 0.30 GB instead of > 0.32 GB
time-mem.bloater:
	OUT=`time-mem -p 1 bloater -maxiter 32 10000000 2>&1`; \
	echo $$OUT; \
	[[ "$$OUT" =~ "Real ([0-9.]+)" ]] && [[ "$${BASH_REMATCH[1]}" > 32 ]] \
	&& [[ "$$OUT" =~ "VMEM ([0-9.]+)" ]] && [[ "$${BASH_REMATCH[1]}" > 0.30 ]]

