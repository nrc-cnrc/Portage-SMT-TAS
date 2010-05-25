# @file Makefile validates various alignment formats with word_align_tool
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

all: cmp_all read_all error
	@echo All tests PASSED.

.SECONDARY:

clean:
	${RM} align.* aligned.1 aligned-in.1 core*

cmp_all: $(addprefix cmp_align.,aachen gale hwa matrix compact ugly green sri uli patterns)

read_all: $(addprefix read_align.,sri green hwa)

cmp_align.%: align.%
	diff $< ref/$<

cmp_align.hwa: align.hwa
	diff aligned.1 ref/aligned.1

export PORTAGE_INTERNAL_CALL=1

align.%:
	word_align_tool -fin sri -fout $* in/text.fr in/text.en in/align.sri $@

read_align.%: align.%
	word_align_tool -fin $* -fout sri in/text.fr in/text.en $< - | \
	   cmp - in/align.sri

read_align.hwa: align.hwa
	cp aligned.1 aligned-in.1
	word_align_tool -fin hwa -fout sri in/text.fr in/text.en aligned.1 - | \
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

