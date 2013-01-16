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

# NOTE / CMDS:
# xpath xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/QP.xml '//trans-unit[not(@translate) and not(self::target)]'

LOGS := .logs

CE_TMX_PL := ce_tmx.pl

CORPORA_DIR := ../../data
XLIFF_DIR   := ${CORPORA_DIR}/sdlxliff
TMX_DIR     := ${CORPORA_DIR}/tmx

XLIFF := $(subst ${XLIFF_DIR}/,,$(shell find ${XLIFF_DIR} -name \*.sdlxliff))
TMX   := $(subst ${TMX_DIR}/,,$(shell find ${TMX_DIR} -name \*.tmx -type f))
DIRS  := $(dir ${XLIFF} ${TMX})

TEMP_DIRS  := tmx xliff
TEMP_FILES := *.tmx *.sdlxliff

-include Makefile.params
-include Makefile.expected_count
include /home/portage/pkgs/gmsl-1.1.1/gmsl  # Needed for EXPECTED_FILTER
include ../Makefile.incl

$(shell mkdir -p ${LOGS})

SHELL = bash

vpath %.sdlxliff ${XLIFF_DIR}
vpath %.tmx ${TMX_DIR}

.SECONDARY:
.DEFAULT_GOAL := all
.PHONY: all
all:  xliff
all:  tmx
all:  testSuite


.PHONY: clean
clean:
	${RM} -r xliff tmx



.PHONY:  xliff tmx

# Perform TMX file extraction.
TMX_LIST := $(foreach c, $(basename ${TMX}), tmx/$c/val)
tmx:  ${TMX_LIST}
tmx/%/Q.txt tmx/%/Q.tags:  %.tmx
	@mkdir -p $(dir $@)
	-@rm $(dir $@)/*
	${CE_TMX_PL} \
		-verbose=1 \
		$(and ${KEEPTAGS}, -keeptags) \
		$(and ${DEBUG}, -d) \
		$(and ${PP}, -pretty_print) \
		extract $(dir $@) $< \
		2> ${LOGS}/$(basename $(subst /,.,$@)).log


# Perform SDLXLIFF file extraction.
XLIFF_LIST :=  $(foreach c, $(basename ${XLIFF}), xliff/$c/val)
xliff:  ${XLIFF_LIST}
xliff/%/Q.txt xliff/%/Q.tags:  %.sdlxliff
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


# Instead of translating, let's reverse the source sentence and pretend it is our translation.
%/P.txt:  %/Q.txt
	! LC_ALL=C egrep '&#x1[eEfF];' $+
	perl -nle 'BEGIN{use encoding "UTF-8";}; print scalar reverse' < $< > $@


# Reversing the tagged source sentence doesn't work when reinserting thus we'll change case instead.
%/P.tags:  %/Q.tags
	! LC_ALL=C egrep '&#x1[eEfF];' $+
	sed 's/^/TRANSLATION: /' $< > $@
	#perl -ple 'BEGIN{use encoding "UTF-8";}; chomp; y/A-Za-z/a-zA-Z/; s/&(LT|AMP);/lc($$1);/ge' < $< > $@


# Let's pretend the line number is our confidence estimation (must be deterministic for unittesting).
%/pr.ce:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print $$./100' < $< > $@


# Insert translation back into either a TMX or a SDLXLIFF file.
%/QP.xml:  %/P.txt  %/pr.ce  %/P.tags
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



# Validate TMX format.
#${TMX_LIST}:  tmx/%/val:  tmx/%/QP.xml  %.tmx
tmx/%/val:  tmx/%/QP.xml  %.tmx
	[[ `xpath <(sed -e 's/<!DOCTYPE tmx SYSTEM "tmx14.dtd">//' < $<) 'count(//tuv[@xml:lang="FR-CA" or @xml:lang="fr-CA"])' 2> /dev/null` -eq $(call EXPECTED_FILTER, ${EXPECTED_SIZE}, $(or ${FILTER_100}, ${FILTER})) ]]
	$(and ${SCORE}, ${FILTER}, [[ "`xpath <(sed -e 's/<!DOCTYPE tmx SYSTEM "tmx14.dtd">//' < $<) 'count(//tu/prop[@type=\"Txt::CE\"])' 2> /dev/null`" -eq $(call EXPECTED_FILTER, ${EXPECTED_SIZE}, $(or ${FILTER_100}, ${FILTER})) ]])
	$(if ${FILTER},, diff --ignore-matching-lines="^<file filename" <(xml_grep --nodate '//tuv[@xml:lang="EN-CA"]' $(filter %/QP.xml, $+)) <(xml_grep --nodate '//tuv[@xml:lang="EN-CA"]' $(filter %.tmx, $+)) \
	|| ! echo "the source sentence was altered" >&2)
	echo "validate that the source sentence is unaltered: viewdiff <(xmllint --format --recover - 2>/dev/null < $(filter %.tmx,$+))  <(xmllint --format --recover - 2>/dev/null < $(<:xml=template.xml))  <(xmllint --format --recover - 2>/dev/null < $<)"

# Validate sdlxliff format.
#$(filter-out xliff/unittest1/val, ${XLIFF_LIST}):  xliff/%/val:  xliff/%/QP.xml
xliff/%/val:  xliff/%/QP.xml  %.sdlxliff
	[[ "`xpath $< 'count(//target[not(@*)]//mrk[@mtype=\"seg\"])' 2> /dev/null`" -eq "$(call EXPECTED_FILTER, ${EXPECTED_SIZE}, ${FILTER_100})" ]] \
	|| ! echo "ERROR: There are some target with attributs" >&2
	[[ "`xpath $< 'count(//target//mrk)' 2> /dev/null`" -eq "$(call EXPECTED_FILTER, ${EXPECTED_SIZE}, ${FILTER_100})" ]] \
	|| ! echo "ERROR: There is not the right number of mrk segments." >&2 
	$(and ${SCORE}, ${FILTER}, [[ "`xpath $< 'count(//sdl:seg-defs/sdl:seg[@percent >=0 and @percent<=100])' 2> /dev/null`" -le "$(call EXPECTED_FILTER, ${EXPECTED_SIZE}, ${FILTER_100})" ]] || ! echo "ERROR: Filtering" >&2)
	$(if ${FILTER},, diff --ignore-matching-lines="^<file filename" <(xml_grep --nodate '//source' $(filter %/QP.xml, $+)) <(xml_grep --nodate '//source' $(filter %.sdlxliff, $+)) \
	|| ! echo "the source sentence was altered" >&2)



########################################
# CHECK
XLIFF_CHECK :=  $(foreach c, $(basename ${XLIFF}), xliff/$c/check)
TMX_CHECK :=  $(foreach c, $(basename ${TMX}), tmx/$c/check)
check:  ${XLIFF_CHECK}  ${TMX_CHECK}

xliff/%/check:  %.sdlxliff
	[[ `${CE_TMX_PL} check $< 2> /dev/null` -eq "$(call EXPECTED_FILTER, ${EXPECTED_SIZE}, ${FILTER_100})" ]]

tmx/%/check:  %.tmx
	[[ `${CE_TMX_PL} check $< 2> /dev/null` -eq "$(call EXPECTED_FILTER, ${EXPECTED_SIZE}, ${FILTER_100})" ]]




########################################
# FUNCTIONS
# ${SCORE} and ${FILTER} ?
#    $1 > $2 ? $1 - $2 + 1 : 0
#    $1 (= EXPECTED_SIZE)
# where $1 = EXPECTED_SIZE
#       $2 = FILTER_100
EXPECTED_FILTER = $(or $(and ${SCORE}, ${FILTER}, $(or $(and $(call gt, $1, $2), $(call inc, $(call subtract, $1, $2))), 0)), $1)



########################################
# UNITTEST'S DEFINITION.
.PHONY:  testSuite

# TODO: Test <seg-source><><><mrk mtype="seg">
.PHONY:  test2 test2s test2sf
test2:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/val
test2s:  SCORE=1
test2s:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/val
test2sf:  SCORE=1
test2sf:  FILTER=0.44
test2sf:  FILTER_100=44
test2sf:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/val


# Create a file to score sdlxliff
%.s.sdlxliff:  %.sdlxliff
	@cp $< $@

# Create a file to score tmx
%.s.tmx:  %.tmx
	@cp $< $@

# Create a file to score and filter sdlxliff
%.s.f.sdlxliff:  %.sdlxliff
	@cp $< $@

# Create a file to score and filter tmx
%.s.f.tmx:  %.tmx
	@cp $< $@


########################################
# Score all SDLXLIFF examples
testSuite:  unittest_sdlxliff_score
.PHONY:  unittest_sdlxliff_score
unittest_sdlxliff_score:  override SCORE = 1
unittest_sdlxliff_score:  $(foreach c,${XLIFF}, xliff/${c:.sdlxliff=.s}/val)

########################################
# Score and filter all SDLXLIFF examples
testSuite:  unittest_sdlxliff_score_filter
.PHONY:  unittest_sdlxliff_score_filter
unittest_sdlxliff_score_filter:  override SCORE = 1
unittest_sdlxliff_score_filter:  override FILTER = 0.14
unittest_sdlxliff_score_filter:  override FILTER_100 = 14
unittest_sdlxliff_score_filter:  $(foreach c,${XLIFF}, xliff/${c:.sdlxliff=.s.f}/val)


########################################
# Score all TMX examples
testSuite:  unittest_tmx_score
.PHONY:  unittest_tmx_score
unittest_tmx_score:  override SCORE=1
unittest_tmx_score:  $(foreach c,${TMX}, tmx/${c:.tmx=.s}/val)


########################################
# Score and filter all TMX examples
testSuite:  unittest_tmx_score_filter
.PHONY:  unittest_tmx_score_filter
unittest_tmx_score_filter:  override SCORE=1
unittest_tmx_score_filter:  override FILTER=0.44
unittest_tmx_score_filter:  override FILTER_100=44
unittest_tmx_score_filter:  $(foreach c,${TMX}, tmx/${c:.tmx=.s.f}/val)


# This test is use to evaluate how well we wrap bpt tags into open_wrap & close_wrap.
testSuite:  unittest_wrap
.PHONY: unittest_wrap
unittest_wrap:  tmx/TestMemory_Unknown/Q.tags
	[[ `egrep -o 'open_wrap' $+ | \wc -l` -eq 21 ]]
	[[ `egrep -o 'close_wrap' $+ | \wc -l` -eq 21 ]]
	[[ `egrep -o 'tag_wrap' $+ | \wc -l` -eq 6 ]]


########################################
# UNITTEST1
# Make sure there is an error message when we are missing one entry.
unittest1.sdlxliff:  UnknownSegments_en-CA_fr-CA.v5.sdlxliff
	@cp $< $@

xliff/unittest1/Q.txt xliff/unittest1/Q.tags:  override EXPECTED_SIZE = 39

# Deletes the first line of P.txt & Q.ix to simulate a missing entry.
xliff/unittest1/QP.xml:  %/QP.xml:  %/P.txt  %/P.tags  %/pr.ce
	sed -i '1d' $*/P.txt $*/Q.ix $*/P.tags
	set -o pipefail; ${CE_TMX_PL} replace $* 2>&1 | egrep --quiet "Can't find ID 0050212b-4890-4b01-9164-dadcc6bddc91.20"

xliff/unittest1/val:  %/val:  %/QP.xml
	@true

testSuite:  unittest1
unittest1:  xliff/unittest1/QP.xml






################################################################################
# HELPERS
########################################
# create the expected count for the number of mrk mtype="seg" in a sdlxliff files.
# create the expected count for the number of tuv xml:lang=English in tmx files.
.PHONY:  count
count:  $(addsuffix .count, ${XLIFF})
count:  $(addsuffix .count, ${TMX})

%.sdlxliff.count:  %.sdlxliff
	@echo "xliff/$*/%:  override EXPECTED_SIZE = `xpath $+ 'count(//seg-source//mrk[@mtype=\"seg\"])' 2> /dev/null`"
	@[[ `xpath $< 'count(//mrk[@mtype="seg"])' 2> /dev/null` -eq `egrep -o '<mrk[^<]* mtype="seg"' $< | \wc -l` ]]

TMX_COUNT_CONVERT := iconv -f UTF-16 -t UTF-8
testmemory.tmx.count:  override TMX_COUNT_CONVERT=cat
%.tmx.count:  %.tmx
	@echo "tmx/$*/%:  override EXPECTED_SIZE = `xml_grep --count '//tuv[@xml:lang="EN-CA" or @xml:lang="en-CA"]' $< | head -1 | cut -f 2 -d:`"
	@[[ `xml_grep --count '//tuv[@xml:lang="EN-CA" or @xml:lang="en-CA"]' $< | head -1 | cut -f 2 -d:` -eq `${TMX_COUNT_CONVERT} < $< | egrep -o '<tuv xml:lang="(en|EN)-CA">' | \wc -l` ]]

