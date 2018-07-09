#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

SHELL := bash

-include Makefile.params

contexts = contexts
canoe_time_old = 10
canoe_time_new = 2
corpora_time_old = 10
corpora_time_new = 2
translation_discard_time = 5
incremental_discard_time = 7

TEMP_DIRS=${contexts}
include ../Makefile.incl

.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite



%.setup:
	# Scenario that doesn't require handling since no folder/files will ever be created for this scenario.
	#mkdir -p ${contexts}/$*/canoe_no.corpora_no
	# Impossible because if there exists a canoe there must be corpora since corpora is what triggers the creation of canoe.ini.
	# Both canoe & corpora exist or they don't.
	#mkdir -p ${contexts}/$*/canoe_new.corpora_no
	#touch -t $$(date --date="${canoe_time_new} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_new.corpora_no/canoe.ini.cow
	#mkdir -p ${contexts}/$*/canoe_old.corpora_no
	#touch -t $$(date --date="${canoe_time_old} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_old.corpora_no/canoe.ini.cow
	#mkdir -p ${contexts}/$*/canoe_no.corpora_new
	#touch -t $$(date --date="${corpora_time_new} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_no.corpora_new/corpora
	#mkdir -p ${contexts}/$*/canoe_no.corpora_old
	#touch -t $$(date --date="${corpora_time_old} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_no.corpora_old/corpora
	mkdir -p ${contexts}/$*/canoe_new.corpora_new
	touch -t $$(date --date="${canoe_time_new} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_new.corpora_new/canoe.ini.cow
	touch -t $$(date --date="${corpora_time_new} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_new.corpora_new/corpora
	mkdir -p ${contexts}/$*/canoe_old.corpora_new
	touch -t $$(date --date="${canoe_time_old} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_old.corpora_new/canoe.ini.cow
	touch -t $$(date --date="${corpora_time_new} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_old.corpora_new/corpora
	mkdir -p ${contexts}/$*/canoe_new.corpora_old
	touch -t $$(date --date="${canoe_time_new} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_new.corpora_old/canoe.ini.cow
	touch -t $$(date --date="${corpora_time_old} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_new.corpora_old/corpora
	mkdir -p ${contexts}/$*/canoe_old.corpora_old
	touch -t $$(date --date="${canoe_time_old} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_old.corpora_old/canoe.ini.cow
	touch -t $$(date --date="${corpora_time_old} days ago" +"%y%m%d%H%M") ${contexts}/$*/canoe_old.corpora_old/corpora


testsuite:  testcase1
testcase1:  testcase_delete_unused_contexts
testcase_delete_unused_contexts:  %:  %.setup
	find ${contexts}/$@ -name canoe.ini.cow -atime +${translation_discard_time} -exec dirname {} \; \
	| sort \
	| diff - ref/$@


testsuite:  testcase2
testcase2:  testcase_delete_contexts_that_didnt_get_new_sentence_pair
testcase_delete_contexts_that_didnt_get_new_sentence_pair:  %:  %.setup
	find ${contexts}/$@ -name corpora -atime +${incremental_discard_time} -exec dirname {} \; \
	| sort \
	| diff - ref/$@


testsuite:  testcase3
testcase3:  testcase_delete_contexts_that_didnt_get_new_sentence_pair_and_hasnt_been_used_for_translation
testcase_delete_contexts_that_didnt_get_new_sentence_pair_and_hasnt_been_used_for_translation:  %:  %.setup
	find ${contexts}/$@ -name canoe.ini.cow -atime +${translation_discard_time} -exec dirname {} \; \
	| grep --line-regexp --fixed-strings --file=<(find ${contexts}/$@ -name corpora -atime +${incremental_discard_time} -exec dirname {} \;) \
	| sort \
	| diff - ref/$@
	find ${contexts}/$@ \( -name canoe.ini.cow -atime +${translation_discard_time} -or -name corpora -atime +${incremental_discard_time} \) -exec dirname {} \; \
	| sort \
	| uniq --repeated \
	| sort \
	| diff - ref/$@
