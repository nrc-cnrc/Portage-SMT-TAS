#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

-include Makefile.params

CE_TMX_PL := ../confidence/ce_tmx.pl

XLIFF := $(shell find TestSet -name \*.sdlxliff -type f) $(wildcard *.sdlxliff)
TMX   := $(shell find TestSet -name \*.tmx -type f) $(wildcard *.tmx)
DIRS  := $(dir ${XLIFF} ${TMX})


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
	-@rm xliff/$*/*
	${CE_TMX_PL} -verbose=1 $(and ${DEBUG}, -d) extract $(dir $@) $<


xliff:  $(foreach c, $(basename ${XLIFF}), xliff/$c/QP.xml)
xliff/%/Q.txt:  %.sdlxliff
	@mkdir -p $(dir $@)
	-@rm xliff/$*/*
	${CE_TMX_PL} -verbose=1 $(and ${DEBUG}, -d) extract $(dir $@) $<


%/P.txt:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print scalar reverse' < $< > $@


%/pr.ce:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print $$.' < $< > $@


%/QP.xml:  %/P.txt  %/pr.ce
	${CE_TMX_PL} \
		-verbose=1 \
		$(and ${DEBUG}, -d) \
		$(and ${PP}, -pretty_print) \
		$(or $(and ${SCORE}, -score), -noscore) \
		$(and ${FILTER}, -filter=${FILTER}) \
		replace $*


########################################
# 
.PHONY:  test
# Simple hand made test
test1:  xliff/test_numbers_hyphens_2.docx/QP.xml
# Test <seg-source><><><mrk mtype="seg">
test2:  xliff/TestSet/Target-sdlxliff/WNv_report_-_WEEK_39_graphs_and_tables_4th_Oct_2012.xlsx/QP.xml


########################################
# 
.PHONY:  rsync
rsync:  xliff/QP.xml
	rsync -arz $< ilt:/home/larkins/exp/Lexitech/xliff/test_numbers_hyphens_2.pretty.translated.sdlxliff


########################################
# 
unittest1.sdlxliff:  test_numbers_hyphens_2.docx.sdlxliff
	cp $< $@

xliff/unittest1/QP.xml:  %/QP.xml:  %/P.txt  %/pr.ce
	sed -i '1d' $*/P.txt $*/Q.ix
	set -o pipefail; ${CE_TMX_PL} replace $* 2>&1 | egrep "Can't find ID 0d314dfb-cbb7-41a5-810e-02207e7d1e9e.13"

unittest1:  xliff/unittest1/QP.xml

