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
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada


STRIPE := stripe.py

-include Makefile.params

$(info Using stripe: $(shell which ${STRIPE}))


SHELL = bash

PYTHON2 := python2
PYTHON3 := python3

.PHONY: all
all: testsuite


TEMP_FILES=input.gz unittest*
include ../Makefile.incl


.PHONY: testsuite
testsuite: unittest1
testsuite: unittest2
testsuite: unittest3
testsuite: unittest4
testsuite: unittest5
testsuite: unittest6
testsuite: unittest7
testsuite: unittest8
testsuite: unittest9
testsuite: unittest10



########################################
# Extract all even lines.
.PHONY: unittest1
unittest1:
	${STRIPE} -i 0 -m 2 <(seq 0 10) | diff - <(seq 0 2 10) -q


########################################
# Extract all odd lines.
.PHONY: unittest2
unittest2:
	${STRIPE} -i 1 -m 2 <(seq 0 10) | diff - <(seq 1 2 10) -q


########################################
# Make sure that we are not losing any lines when extracting even lines and extracting odd lines.
.PHONY: unittest3
unittest3:
	[[ `cat <(${STRIPE} -i 0 -m 2 <(seq 0 10)) <(${STRIPE} -i 1 -m 2 <(seq 0 10)) | \wc -l` == 11 ]] || ! echo "We are missing some entries!" >&2


########################################
# Validate the index based on the modulo.
.PHONY: unittest4
unittest4:
	${STRIPE} -i 2 -m 2 <(seq 0 10) 2>&1 | grep -qF 'Indices format is -i i:j where [i,j) where 0 <= i < j <= m.'


########################################
# Extract all odd lines from standard input.
.PHONY: unittest5
unittest5:
	seq 0 10 | ${STRIPE} -i 1 -m 2 | diff - <(seq 1 2 10) -q


########################################
# Extract all odd lines from compressed file.
.PHONY: unittest6
unittest6:  input.gz
	${STRIPE} -i 1 -m 2 $+ | diff - <(seq 1 2 10) -q


########################################
# Rebuild from stipe output.
.PHONY: unittest7  unittest7p unittest7l
unittest7: unittest7p
unittest7: unittest7l
unittest7: unittest7u
unittest7: unittest7ulimit

CHUNK_LIST := 0 1 2 3 4 5 6 7 8 9 10
GEN := seq 0 43
# Rebuild from a pattern.
unittest7p: $(addprefix unittest7., ${CHUNK_LIST})
	${STRIPE} -r unittest7. | diff - <(${GEN}) -q

# Rebuild from a list of files.
unittest7l: $(addprefix unittest7., ${CHUNK_LIST})
	${STRIPE} -r $+ | diff - <(${GEN}) -q

# Unsupported feature.
unittest7u: $(addprefix unittest7., ${CHUNK_LIST})
	${STRIPE} -r unittest7. -i 0:2 2>&1 \
	| grep -qF 'Not implemented yet!  You can only merge if you used -i without a range.'

# Too many file for ulimit
# NOTE: we need a few file handle for STDs and libs but we want less file handles than it would require to open all chunks.
unittest7ulimit: $(addprefix unittest7., ${CHUNK_LIST})
	-ulimit -n 9; \
	${STRIPE} -r $+ 2>&1 \
	| grep -qF "You provided 11 files to merge but the os doesn't allow that many file to be opened at once."


unittest7.%:
	${STRIPE} -i $* -m 11 <(${GEN}) $@



########################################
# Basically extract all content with a full range of index within the modulo
.PHONY: unittest8
unittest8:
	${STRIPE} -i 0:2 -m 2 <(${GEN}) | diff - <(${GEN}) -q



########################################
# Exclude one of three lines
.PHONY: unittest9
unittest9:
	${STRIPE} -i 0:2 -m 3 <(seq 0 10) | diff - <(sort -g <(seq 0 3 10) <(seq 1 3 10)) -q


# invalid.utf8 contains some invalid utf-8 on the first line.  Python will crap
# out on the first line stating that there is an invalid utf8 regardless of -i.
# Thus, it is sufficient to run stripe.py with any value of -i for this test.
# Note that using stdin/stdout is clearly a different testcase than using filenames.
.PHONY: unittest10
.PHONY: unittest10_stdin_py2  unittest10_files_py2  unittest10_rebuild_py2
.PHONY: unittest10_stdin_py3  unittest10_files_py3  unittest10_rebuild_py3
unittest10:  unittest10_stdin_py2
unittest10:  unittest10_files_py2
unittest10:  unittest10_rebuild_py2
ifneq ($(strip $(shell which ${PYTHON3} 2> /dev/null)),)
unittest10:  unittest10_stdin_py3
unittest10:  unittest10_files_py3
unittest10:  unittest10_rebuild_py3
else
$(info Skipping python3 testcases.)
endif
unittest10:
	@true

%_py2:  MY_PYTHON = ${PYTHON2}
%_py3:  MY_PYTHON = ${PYTHON3}

unittest10_stdin_py2  unittest10_stdin_py3:  src/invalid.utf8
	${MY_PYTHON} `which ${STRIPE}` -i 0 -m 3 < $< \
	| diff - ref/invalid.utf8 --brief

unittest10_files_py2  unittest10_files_py3:  src/invalid.utf8
	${MY_PYTHON} `which ${STRIPE}` -i 0 -m 3 $< $@
	diff $@ ref/invalid.utf8 --brief

unittest10_rebuild_py2  unittest10_rebuild_py3:  src/invalid.utf8
	${MY_PYTHON} `which ${STRIPE}` -r $< $< $< \
	| diff - ref/invalid.utf8.rebuild --brief





################################################################################
# HELPERS
input.gz:
	seq 0 10 | gzip > $@
