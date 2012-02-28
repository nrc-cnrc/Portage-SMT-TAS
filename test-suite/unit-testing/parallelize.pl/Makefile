#!/usr/bin/make -f
# vim:noet:ts=3:nowrap
# Makefile - Unit-test for parallelize.pl.
# Tests
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

# For this TestSuite, we assume that all software to run distributed on the
# cluster are already tested and verified.
export PORTAGE_NOCLUSTER=1

SHELL = /bin/bash

PARALLELIZE_PL ?= parallelize.pl

vpath %.txt ${PORTAGE}/test-suite/toy  ${PORTAGE}/test-suite/unit-testing/toy
vpath input% src

.PHONY: all  testsuite
all:  testsuite


.PHONY: clean
clean:
	${RM} run-parallel-logs-*  log.*
	${RM} -r parallelize.pl.* run-p.[0-9]*
	${RM} -r output* striped_mode1



########################################
# Default functions
.PHONY: split_worker split_line striped_mode2
striped_mode2 split_worker split_line:  %:  output.%
	cmp <(tr '[1-9]' '[a-i]' < src/input) $+
	cmp <(tr '[1-9]' '[a-i]' < src/input | tr "[a-z]" "[A-Z]") <(zcat $+.gz)

output.%:  input input.gz
	@echo "PORTAGE_NOCLUSTER = $$PORTAGE_NOCLUSTER" >&2
	${PARALLELIZE_PL} \
		${EXTRA} \
		-s '$+' \
		-m '$@ $@.gz' \
		"./process.pl $(filter %input, $+) $@ $(filter %input.gz, $+) $@.gz" \
		2> log.$@


########################################
# Split based on workers or window's size.
testsuite: split

.PHONY: split
split: split_worker split_line


# Split according to the number of worker.
output.split_worker:  EXTRA = -n 2


# Split according to a specific number of line.
output.split_line:  EXTRA = -w 3



########################################
# Make sure if the user provides more than one argument to -s or -m that we
# split it correctly.
testsuite: check_s_m_list

.PHONY: check_s_m_list
check_s_m_list:
	OUT=`${PARALLELIZE_PL} -show_args -d -s 'a b		c c  ' -m 'x 	y' -m z 'cat a b c x y z < d > e' 2>&1`; \
	[[ "$$OUT" =~ "CMD=cat a b c x y z < d > e 2> /dev/stderr" ]] \
	&& [[ "$$OUT" =~ 'SPLITS=a : b : c : d' ]] \
	&& [[ "$$OUT" =~ "MERGES=/dev/stderr : e : x : y : z" ]] 



########################################
# Unittest to verify that we are indeed capable of merging more files than the
# command line would allow.
testsuite: numerous_chunks

output:
	mkdir -p $@

.PHONY: numerous_chunks
.PHONY: numerous_chunks0 numerous_chunks1 numerous_chunks2
numerous_chunks: numerous_chunks0
numerous_chunks: numerous_chunks1
numerous_chunks: numerous_chunks2

numerous_chunks0:  output/numerous_chunks0.gz
numerous_chunks1:  output.numerous_chunks1
numerous_chunks2:  output.numerous_chunks2
numerous_chunks0 numerous_chunks1 numerous_chunks2:  hans3_fr.txt
	zcmp $+

output/numerous_chunks0.gz:  | output
# Output to some file.
output/numerous_chunks0.gz output.numerous_chunks1:  hans3_fr.txt
	${PARALLELIZE_PL} -n 5000 -np 4 "cat < $+ > $@" 2> log.$(basename $(notdir $@))

# Output to stdout.
output.numerous_chunks2:  hans3_fr.txt
	${PARALLELIZE_PL} -n 5000 -np 4 "cat < $+" > $@ 2> log.$@



########################################
# Unittest to validate the stip mode which use psplit.py to prevent creating
# temporary intermediate files.
#testsuite: striped_mode

.PHONY: striped_mode
striped_mode: striped_mode1
striped_mode: striped_mode2

# In strip mode there shouldn't be any input file created other than the .done files.
.PHONY: striped_mode1
striped_mode1: %:  output.%.gz
	[[ `grep -c split.py $*/commands` == 2 ]] || ! echo "Looks like stripe mode wasn't used!" >&2
	grep -q 'xargs split.py -r' $*/commands.merge || ! echo "Looks like we didn't use the proper merge command." >&2
	[[ `find $* -size 0 -name \*.done | \wc -l` == 2 ]] || ! echo "You should have 2 empty files." >&2
	cmp src/input <(zcat -f $+)

# Must be in debug mode in order to validate the stripe command file.
output.striped_mode1.gz: output.%.gz:  input
	${PARALLELIZE_PL} -d -n 2 -stripe -workdir="$*" 'cat < $+ > $@' 2> log.$(basename $@)

# Split using stripe mode.
output.striped_mode2:  EXTRA = -n 2 -stripe

