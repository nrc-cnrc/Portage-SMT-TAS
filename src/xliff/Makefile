#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

-include Makefile.params

CE_TMX_PL := ../confidence/ce_tmx.pl

#.INTERMEDIATE:
#.PRECIOUS:
.SECONDARY:
.DEFAULT_GOAL := all
.PHONY: all
all:


.PHONY: clean
clean:

.PHONY:  xliff tmx
xliff tmx:  %:  %/QP.xml
	cat $<

tmx/Q.txt:  testmemory.tmx
xliff/Q.txt:  test_numbers_hyphens_2.pretty.sdlxliff

%/Q.txt:
	-@rm xliff/*;
	${CE_TMX_PL} extract $* $<


%/P.txt:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print scalar reverse' < $< > $@


%/pr.ce:  %/Q.txt
	perl -nle 'BEGIN{use encoding "UTF-8";}; print $$.' < $< > $@


%/QP.xml:  %/P.txt  %/pr.ce
	${CE_TMX_PL} -d $(or $(and ${SCORE}, -score), -noscore) $(and ${FILTER}, -filter=10) replace $*


.PHONY:  rsync
rsync:  xliff/QP.xml
	rsync -arz $< ilt:/home/larkins/exp/Lexitech/xliff/test_numbers_hyphens_2.pretty.translated.sdlxliff
