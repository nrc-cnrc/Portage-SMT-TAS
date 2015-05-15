# @file Makefile validates various alignment formats with word_align_tool
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

all: cmp_all cmp_all2 read_all error

TEMP_FILES=align.* align2.* aligned.1 aligned-in.1
include ../Makefile.incl

export PORTAGE_INTERNAL_CALL=1

.SECONDARY:

# hwa handling is not parallelisable, and everything is quick, so just don't do
# parallel.
.NOTPARALLEL:

cmp_all: $(addprefix cmp_align.,aachen gale hwa matrix compact ugly green sri uli patterns bilm)

cmp_all2: $(addprefix cmp_align2.,aachen gale hwa matrix compact ugly green sri uli patterns bilm)

read_all: $(addprefix read_align.,sri green hwa)

cmp_align%: align%
	diff $< ref/$<

align.%:
	word_align_tool -fin sri -fout $* in/text.fr in/text.en in/align.sri $@

align.hwa:
	word_align_tool -fin sri -fout hwa in/text.fr in/text.en in/align.sri $@
	mv aligned.1 align.hwa

align2.%:
	word_align_tool -fin green -fout $* in/test2.en in/test2.fr in/align2.green $@

align2.hwa:
	word_align_tool -fin green -fout hwa in/test2.en in/test2.fr in/align2.green $@
	mv aligned.1 align2.hwa

read_align.%: align.%
	word_align_tool -fin $* -fout sri in/text.fr in/text.en $< - | \
	   cmp - in/align.sri

read_align.hwa: align.hwa
	cp align.hwa aligned-in.1
	word_align_tool -fin hwa -fout sri in/text.fr in/text.en align.hwa - | \
	   cmp - in/align.sri

error:
	word_align_tool -fin sri -fout sri in/text.en in/text.en in/align.sri - > /dev/null ; \
	RC=$$? ; \
	if [[ $$RC = 1 ]]; then \
	   echo Note: the error above means word_align_tool is working correctly. ; \
	else \
	   echo Error: word_align_tool got exit status $$RC ; \
	   echo Error: word_align_tool should have shown an error message with exit status 1. ; \
	   false ; \
	fi

