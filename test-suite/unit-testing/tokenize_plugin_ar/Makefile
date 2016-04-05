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

TOKENIZE_PLUGIN ?= tokenize_plugin_ar
SENTSPLIT_PLUGIN ?= sentsplit_plugin

-include Makefile.params

vpath %_ar  src/

SHELL := bash
.DEFAULT_GOAL := all
.PHONY: all


TEMP_FILES = *.tok

include ../Makefile.incl


# Let's tokenize an input that contains nothing not even a newline.
.PHONY:  empty.validate
all:  empty.validate
empty.validate:  empty.tok
	[[ ! -s $< ]]
empty.tok:
	echo -n "" | ${TOKENIZE_PLUGIN} ar > $@

# Let's tokenize an empty line.
.PHONY:  empty_line.validate
all:  empty_line.validate
empty_line.validate:  empty_line.tok
	[[ `\wc -l < $<` -eq 1 ]]  # There is oneline
	[[ `grep -c '^$$' < $<` -eq 1 ]]  # There is exactly one empty line
empty_line.tok:
	${TOKENIZE_PLUGIN} ar <<< "" > $@

# During development, Arabic underscore were causing problems.
.PHONY:  underscore.validate
all:  underscore.validate
underscore.validate:  underscore.tok
	[[ `\wc -l < $<` -eq 1 ]]
underscore.tok:
	${TOKENIZE_PLUGIN} ar <<< "ـــــــــــــــــــــــ"  > $@

# This is the original input that was failing where a multisentence line was
# passed to ${SENTSPLIT_PLUGIN} which added an empty line at the end of the
# document then ${TOKENIZE_PLUGIN} was dying on that empty line.
.PHONY:  multisentence.validate
all:  multiSentence.validate
multiSentence.validate:  multiSentence.tok
	[[ `\wc -l < $<` -eq 12 ]]
multiSentence.tok:  Fan_ar
	${SENTSPLIT_PLUGIN} ar < $< | ${TOKENIZE_PLUGIN} ar > $@


# Validate an input containing zero.
.PHONY:  zero.validate
all:  zero.validate
zero.validate:  zero.tok
	[[ `\wc -l < $<` -eq 1 ]]
	diff $< <(echo "0") --brief
zero.tok:
	${TOKENIZE_PLUGIN} ar <<< "0"  > $@


# Validate marking non Arabic words.
.PHONY:  markNonArabicWord.validate
all:  markNonArabicWord.validate
markNonArabicWord.validate:  markNonArabicWord.tok
	[[ `\wc -l < $<` -eq 1 ]]
	diff $< <(echo "__ascii__This __ascii__is __ascii__a k __ascii__test.") --brief
markNonArabicWord.tok:
	${TOKENIZE_PLUGIN} ar -n <<< "This is a ؼtest."  > $@



# Validate marking hashtags.
.PHONY:  hashtags.validate
all:  hashtags.validate
hashtags.validate:  hashtags.tok
	[[ `\wc -l < $<` -eq 1 ]]
	diff $< <(echo "rt @syria_post : <hashtag> AnfwjrAfyk </hashtag> : swryA w+ gzp jrH wAHd <hashtag> swryA </hashtag> <hashtag> gzp </hashtag> <hashtag> gzp tHt AlqSf </hashtag> http://t.co/griwz71tq9") --brief
hashtags.tok:
	${TOKENIZE_PLUGIN} ar -m <<< "RT @syria_post: #انفوجرافيك : سوريا وغزة جرح واحد   #سوريا #غزة #غزة_تحت_القصف http://t.co/grIWZ71tq9"  > $@

# Validate not marking hashtags.
.PHONY:  nohashtags.validate
all:  nohashtags.validate
nohashtags.validate:  nohashtags.tok
	[[ `\wc -l < $<` -eq 1 ]]
	diff $< <(echo "rt @syria_post : # AnfwjrAfyk : swryA w+ gzp jrH wAHd # swryA # gzp # gzp _ tHt _ AlqSf http://t.co/griwz71tq9") --brief
nohashtags.tok:
	${TOKENIZE_PLUGIN} ar <<< "RT @syria_post: #انفوجرافيك : سوريا وغزة جرح واحد   #سوريا #غزة #غزة_تحت_القصف http://t.co/grIWZ71tq9"  > $@



# Validate removing beginning of sentence w+.
.PHONY:  wa.validate
all:  wa.validate
wa.validate:  wa.tok
	[[ `\wc -l < $<` -eq 1 ]]
	diff $< <(echo "yTyb AlHb fy tlk AlrbA mvl +mA Alsyf A*A mst yTyb # swryA") --brief
wa.tok:
	${TOKENIZE_PLUGIN} ar -w <<< "ويطيب الحب في تلك الربا مثلما السيف إذا مست يطيب #سوريا"  > $@

# Validate NOT removing beginning of sentence w+.
.PHONY:  nowa.validate
all:  nowa.validate
nowa.validate:  nowa.tok
	[[ `\wc -l < $<` -eq 1 ]]
	diff $< <(echo "w+ yTyb AlHb fy tlk AlrbA mvl +mA Alsyf A*A mst yTyb # swryA") --brief
nowa.tok:
	${TOKENIZE_PLUGIN} ar <<< "ويطيب الحب في تلك الربا مثلما السيف إذا مست يطيب #سوريا"  > $@


