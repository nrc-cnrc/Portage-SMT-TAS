#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Testsuite for IdentityAligner.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada



-include Makefile.params

# Change the default shell for a better one.
SHELL = bash

GEN_PHRASE_TABLES ?= gen_phrase_tables

ALIGN_WORDS ?= align-words

.PHONY: all
all: testsuite


TEMP_FILES=truecase_map1.lc2tc truecase_map5.lc2tc
include ../Makefile.incl



########################################
.PHONY: testsuite
testsuite: align_words
testsuite: gen_phrase_tables



########################################
# gen_phrase_tables
.PHONY: gen_phrase_tables
gen_phrase_tables: g_equal g_more g_less

.PHONY: IdentityAligner
IdentityAligner:
	${GEN_PHRASE_TABLES} \
		-a 'IdentityAligner' \
		-j -v -m 3 -ibm 1 -1 tc -2 lc \
		/dev/null /dev/null \
		<(echo "A B C D") <(echo "a b c d") \
		<(echo "E F G H I") <(echo "e f g h") \
		<(echo "J K L M") <(echo "j k l m n")

########################################
# Create a joint frequency counts when src.length == tgt.length
.PHONY: g_equal
g_equal:
	${GEN_PHRASE_TABLES} -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 tc -2 lc /dev/null /dev/null \
		<(echo "A B C D") <(echo "a b c d") 2> /dev/null \
		| diff - ref/$@ -q

########################################
# Create a joint frequency counts when src.length > tgt.length
.PHONY: g_more
g_more:
	${GEN_PHRASE_TABLES} -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 tc -2 lc /dev/null /dev/null \
		<(echo "E F G H I J K") <(echo "e f g h") 2> /dev/null \
		| diff - ref/$@ -q

########################################
# Create a joint frequency counts when src.length < tgt.length
.PHONY: g_less
g_less:
	${GEN_PHRASE_TABLES} -a 'IdentityAligner' -j -v -m 3 -ibm 1 -1 tc -2 lc /dev/null /dev/null \
		<(echo "J K L M") <(echo "j k l m n o p") 2> /dev/null \
		| diff - ref/$@ -q

########################################
# Create a simple truecase map since the IdentityAligner was create to try a
# trigram truecasing map.
.PHONY: truecase_map1.lc2tc
testsuite: truecase_map1.lc2tc
truecase_map1.lc2tc: %.lc2tc:
	${GEN_PHRASE_TABLES} -a 'IdentityAligner' -s 'RFSmoother' -multipr fwd -o $* -v -m 1 -ibm 1 -1 lc -2 tc /dev/null /dev/null \
		<(echo "aa aa aa aa aa") <(echo "aa Aa aA AA AA")
	diff $@ ref/$@ -q

########################################
# Create a more elaborate truecasing map using up to 5grams.
.PHONY: truecase_map5.lc2tc
testsuite: truecase_map5.lc2tc
truecase_map5.lc2tc: %.lc2tc:
	${GEN_PHRASE_TABLES} -a 'IdentityAligner' -s 'RFSmoother' -multipr fwd -o $* -v -m 5 -ibm 1 -1 lc -2 tc /dev/null /dev/null \
		<(echo "aa aa aa aa aa") <(echo "aa Aa aA AA AA")
	diff $@ ref/$@ -q



########################################
# align-words
.PHONY: align_words
align_words: a_equal a_more a_less

.PHONY: align-words
align-words:
	${ALIGN_WORDS} \
		-o matrix \
		-a IdentityAligner \
		-ibm 1 \
		/dev/null /dev/null \
		<(echo "A B C D") <(echo "a b c d") \
		<(echo "E F G H I J K") <(echo "e f g h") \
		<(echo "J K L M") <(echo "j k l m n o p")

########################################
# Create a matrix that as src.lenght == tgt.length
.PHONY: a_equal
a_equal:
	A=`${ALIGN_WORDS} -o ugly -a IdentityAligner -ibm 1 /dev/null /dev/null <(echo "A B C D") <(echo "a b c d") 2> /dev/null` \
	&& [[ "$$A" =~ "A/a B/b C/c D/d" ]]

########################################
# Create a matrix that as src.lenght > tgt.length
.PHONY: a_more
a_more:
	A=`${ALIGN_WORDS} -o ugly -a IdentityAligner -ibm 1 /dev/null /dev/null <(echo "E F G H I J") <(echo "e f g h") 2> /dev/null` \
	&& [[ "$$A" =~ "E/e F/f G/g H/h" ]] \
	&& [[ "$$A" =~ "EXCLUDED: 4 5 /" ]]

########################################
# Create a matrix that as src.lenght < tgt.length
.PHONY: a_less
a_less:
	A=`${ALIGN_WORDS} -o ugly -a IdentityAligner -ibm 1 /dev/null /dev/null <(echo "E F G H") <(echo "e f g h i j") 2> /dev/null` \
	&& [[ "$$A" =~ "E/e F/f G/g H/h" ]] \
	&& [[ "$$A" =~ "EXCLUDED: / 4 5" ]]

