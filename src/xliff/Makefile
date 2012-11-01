#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make
# $Id:$

# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


LOGS := .logs

CE_TMX_PL := ../confidence/ce_tmx.pl

XLIFF := $(shell find TestSet -name \*.sdlxliff -type f) $(wildcard *.sdlxliff)
TMX   := $(shell find TestSet -name \*.tmx -type f) $(wildcard *.tmx)
DIRS  := $(dir ${XLIFF} ${TMX})

-include Makefile.params
-include Makefile.expected_count

$(shell mkdir -p ${LOGS})

.SECONDARY:
.DEFAULT_GOAL := all
.PHONY: all
all:  xliff
all:  tmx


.PHONY: clean
clean:

.PHONY:  xliff tmx
#xliff tmx:  %:  %/QP.xml
#	cat $<

#tmx/Q.txt:  testmemory.tmx
#xliff/Q.txt:  test_numbers_hyphens_2.pretty.sdlxliff

tmx:  $(foreach c, $(basename ${TMX}), tmx/$c/QP.xml)
tmx/%/Q.txt:  %.tmx
	@mkdir -p $(dir $@)
	-@rm $(dir $@)/*
	${CE_TMX_PL} \
		-verbose=1 \
		$(and ${KEEPTAGS}, -keeptags) \
		$(and ${DEBUG}, -d) \
		$(and ${PP}, -pretty_print) \
		extract $(dir $@) $< \
		2> ${LOGS}/$(basename $(subst /,.,$@)).log


xliff:  $(foreach c, $(basename ${XLIFF}), xliff/$c/QP.xml)
xliff/%/Q.txt:  %.sdlxliff
	@mkdir -p $(dir $@)
	-@rm $(dir $@)/*
	${CE_TMX_PL} \
		-verbose=1 \
		$(and ${KEEPTAGS}, -keeptags) \
		$(and ${DEBUG}, -d) \
		$(and ${PP}, -pretty_print) \
		extract $(dir $@) $< \
		2> ${LOGS}/$(basename $(subst /,.,$@)).log
	[[ `\wc -l < $@` -eq ${EXPECTED_SIZE} ]]


%/P.txt:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print scalar reverse' < $< > $@


%/pr.ce:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print $$.' < $< > $@


%/QP.xml:  %/P.txt  %/pr.ce
	${CE_TMX_PL} \
		-verbose=1 \
		$(and ${KEEPTAGS}, -keeptags) \
		$(and ${DEBUG}, -d) \
		$(and ${PP}, -pretty_print) \
		$(or $(and ${SCORE}, -score), -noscore) \
		$(and ${FILTER}, -filter=${FILTER}) \
		replace $* \
		2> ${LOGS}/$(basename $(subst /,.,$@)).log
	[[ -s $@ ]] || ! echo "ERROR replacing $@"


########################################
# TEST
.PHONY:  test1 test2
# Simple hand made test
test1:  KEEPTAGS=1
test1:  xliff/test_numbers_hyphens_2.docx/QP.xml
# Test <seg-source><><><mrk mtype="seg">
test2:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/QP.xml



########################################
# UNITTEST1
# Make sure there is an error message when we are missing one entry.
unittest1.sdlxliff:  test_numbers_hyphens_2.docx.sdlxliff
	cp $< $@

# Deletes the first line of P.txt & Q.ix to simulate a missing entry.
xliff/unittest1/QP.xml:  %/QP.xml:  %/P.txt  %/pr.ce
	sed -i '1d' $*/P.txt $*/Q.ix
	set -o pipefail; ${CE_TMX_PL} replace $* 2>&1 | egrep "Can't find ID 0d314dfb-cbb7-41a5-810e-02207e7d1e9e.13"

unittest1:  xliff/unittest1/QP.xml



########################################
# 
.PHONY:  rsync
rsync:  xliff/QP.xml
	rsync -arz $< ilt:/home/larkins/exp/Lexitech/xliff/test_numbers_hyphens_2.pretty.translated.sdlxliff



########################################
# create the expected count for the number of mrk mtype="seg" in a sdlxliff file.
.PHONY:  count
count:  $(addsuffix .count, $(basename ${XLIFF}))
%.count: %.sdlxliff
	@echo "xliff/$*/Q.txt:  override EXPECTED_SIZE = `xpath $+ 'count(//seg-source//mrk[@mtype=\"seg\"])' 2> /dev/null`"
	@[[ `xpath $+ 'count(//mrk[@mtype="seg"])' 2> /dev/null` -eq `egrep -o '<mrk[^<]* mtype="seg"' $+ | \wc -l` ]]
