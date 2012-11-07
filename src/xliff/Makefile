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
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada


LOGS := .logs

CE_TMX_PL := ../confidence/ce_tmx.pl

XLIFF := $(shell find TestSet -name \*.sdlxliff -type f) $(wildcard *.sdlxliff)
TMX   := $(shell find TestSet -name \*.tmx -type f) $(wildcard *.tmx)
DIRS  := $(dir ${XLIFF} ${TMX})

-include Makefile.params
-include Makefile.expected_count
include gmsl/gmsl  # Needed for EXPECTED_FILTER

$(shell mkdir -p ${LOGS})

SHELL = bash
.SECONDARY:
.DEFAULT_GOAL := all
.PHONY: all
all:  xliff
all:  tmx


.PHONY: clean
clean:

.PHONY:  xliff tmx

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


XLIFF_LIST :=  $(foreach c, $(basename ${XLIFF}), xliff/$c/val)
xliff:  ${XLIFF_LIST}
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



# Validate sdlxliff format.
${XLIFF_LIST}:  xliff/%/val:  xliff/%/QP.xml
	xmllint --noout $<
	[[ "`xpath $< 'count(//target[not(@*)]//mrk[@mtype=\"seg\"])' 2> /dev/null`" -eq ${EXPECTED_SIZE} ]] || ! echo "ERROOR: There are some target with attributs" &>2
	[[ "`xpath $< 'count(//target//mrk)' 2> /dev/null`" -eq ${EXPECTED_SIZE} ]]  || ! echo "ERROR: There is not the right number of mrk segments." &>2 
	$(and ${SCORE}, ${FILTER}, [[ "`xpath $< 'count(//sdl:seg-defs/sdl:seg[@percent >=0 and @price<=100])' 2> /dev/null`" -le "$(call EXPECTED_FILTER, ${EXPECTED_SIZE}, ${FILTER})" ]] || ! echo "ERROR: Filtering" &>2)



########################################
# FUNCTIONS
EXPECTED_FILTER = $(or $(and $(call gt, $1, $2), $(call inc, $(call subtract, $1, $2))), 0)



########################################
# TEST
.PHONY:  test1 test2
# Simple hand made test
test1:  KEEPTAGS=1
test1:  xliff/test_numbers_hyphens_2.docx/QP.xml

# Test <seg-source><><><mrk mtype="seg">
test2:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/QP.xml
test2s:  SCORE=1
test2s:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/QP.xml
test2sf:  SCORE=1
test2sf:  FILTER=53
test2sf:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/QP.xml

# Source sdlxliff file doesn't have target.
# Verify that filtering results in 15 = (58 - 44 + 1) mrk segments.
test3:  override SCORE=1
test3:  override FILTER=44
test3:  xliff/TestSet/Unknown-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012/val
	[[ "`xpath $@ 'count(//sdl:seg-defs/sdl:seg/@percent)' 2> /dev/null`" -le "15" ]] || ! echo "ERROR: Filtering" &>2

# Score and filter all xliff examples
test4:  override SCORE=1
test4:  override FILTER=44
test4:  ${XLIFF_LIST}


########################################
# UNITTEST1
# Make sure there is an error message when we are missing one entry.
unittest1.sdlxliff:  test_numbers_hyphens_2.docx.sdlxliff
	@cp $< $@

# Deletes the first line of P.txt & Q.ix to simulate a missing entry.
xliff/unittest1/QP.xml:  %/QP.xml:  %/P.txt  %/pr.ce
	sed -i '1d' $*/P.txt $*/Q.ix
	set -o pipefail; ${CE_TMX_PL} replace $* 2>&1 | egrep "Can't find ID 0d314dfb-cbb7-41a5-810e-02207e7d1e9e.13"

unittest1:  xliff/unittest1/QP.xml



########################################
# 
.PHONY:  rsync
VERSION := $(shell git log --pretty=format:'%h' -n 1)
rsync:  xliff/UnknownSegments_en-CA_fr-CA/rsync
rsync:  xliff/UnknownSegments_en-CA_fr-CA-2/rsync
xliff/%/rsync:  xliff/%/QP.xml
	rsync -arz $< ilt:~/exp/Lexitech/xliff/$*.${VERSION}.sdlxiff



########################################
# create the expected count for the number of mrk mtype="seg" in a sdlxliff file.
.PHONY:  count
count:  $(addsuffix .count, $(basename ${XLIFF}))
%.count: %.sdlxliff
	@echo "xliff/$*/%:  override EXPECTED_SIZE = `xpath $+ 'count(//seg-source//mrk[@mtype=\"seg\"])' 2> /dev/null`"
	@[[ `xpath $+ 'count(//mrk[@mtype="seg"])' 2> /dev/null` -eq `egrep -o '<mrk[^<]* mtype="seg"' $+ | \wc -l` ]]

