# @file Makefile to test markup_canoe_output
# @author Darlene Stewart and Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

.SECONDARY:

.PHONY: all

TAGS_XFER_TESTS = whitespace_test nested1_test nested2_test \
                  escape_test escape-tmx_test paired_test emptytags_test oov_test

TEMP_FILES=$(TAGS_XFER_TESTS:_test=.P.tags.tok) log.* tc_tmp_text_28315.cmark.out wal-*.out
include ../Makefile.incl

export PORTAGE_INTERNAL_CALL=1

########################################
# Markup transfer tests for transfering TMS/XLIFF tags markup

all: ${TAGS_XFER_TESTS}

whitespace_test: OPTS=-v -xtags
nested1_test: OPTS=-v -xtags
nested2_test: OPTS=-v -xtags
escape-tmx_test: OPTS=-v -xtags
escape_test: OPTS=-v -xtags
paired_test: OPTS=-v -xtags
emptytags_test: OPTS=-v -xtags
oov_test: OPTS=-v -xtags

.PHONY: ${TAGS_XFER_TESTS}

${TAGS_XFER_TESTS}: %_test: in/%.Q.tags.tok in/%.P.tok in/%.p.pal
	markup_canoe_output ${OPTS} $+ > $*.P.tags.tok 2> log.$*
	diff ref/$*.P.tags.tok $*.P.tags.tok -q

# Create correctly tokenized input
$(foreach TST, $(TESTS:_test=), in/${TST}.Q.tags.tok): in/%.Q.tags.tok: in/%.Q.tags.txt
	utokenize.pl -noss -xtags $< > $@

########################################
# Regression test for transfering markup for truecasing

all: compare.tc_tmp_text_28315.cmark.out

tc_tmp_text_28315.cmark.out: in/tc_tmp_text_28315.cmark in/tc_tmp_text_28315.tc in/testTC.out.pal
	markup_canoe_output -n OOV $+ 2> log.$@ > $@

compare.%: %
	diff ref/$* $* -q


########################################
# Make sure nbest2rescore.pl produces the files we need for markup_canoe_output to work

all: nbest2rescore_test

nbest2rescore_test: \
   nbest2rescore_check_trace \
   nbest2rescore_check_trace-wal \
   nbest2rescore_check_trace-only
   #nbest2rescore_check_trace-old

nbest2rescore_check_%: nbest2rescore.out.% nbest2rescore.palout.% nbest2rescore.walout.% nbest2rescore.ffout.%
	@true

# Default: the info is available in the canoe output we're reading
nbest2rescore.out.%: in/testTC.out.%
	nbest2rescore.pl -canoe -tagoov -in=$< | diff ref/testTC.out.oov - -q
nbest2rescore.palout.%: in/testTC.out.%
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -palout=- | diff ref/testTC.out.pal - -q
nbest2rescore.walout.%: in/testTC.out.%
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -wal -palout=- | diff ref/testTC.out.wal - -q
nbest2rescore.ffout.%: in/testTC.out.%
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -ffout=- | diff ref/testTC.out.ff - -q

# Exceptions: the info requested is not available in the canoe output we're reading
nbest2rescore.walout.trace nbest2rescore.walout.trace-only nbest2rescore.walout.trace-old: nbest2rescore.walout.%: in/testTC.out.%
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -wal -palout=- | diff ref/testTC.out.wal-missing - -q
nbest2rescore.ffout.trace-only: nbest2rescore.ffout.%: in/testTC.out.%
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -ffout=- | diff ref/testTC.out.ff-missing - -q

# Create the default reference files by calling "make best2rescore_check_ref"
nbest2rescore.out.ref: in/testTC.out.trace-wal
	nbest2rescore.pl -canoe -tagoov -in=$< > ref/testTC.out.oov
nbest2rescore.palout.ref: in/testTC.out.trace-wal
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -palout=- > ref/testTC.out.pal
nbest2rescore.walout.ref: in/testTC.out.trace-wal
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -wal -palout=- > ref/testTC.out.wal
nbest2rescore.ffout.ref: in/testTC.out.trace-wal
	nbest2rescore.pl -canoe -tagoov -in=$< -out=/dev/null -ffout=- > ref/testTC.out.ff


########################################
# Test markup_canoe_output reading WAL info from the PAL file

all: compare.wal-in-pal.out compare.wal-using-heuristic.out

wal-in-pal.out: in/wal.Q.tags.tok in/wal.p.tok in/wal.p.pal
	markup_canoe_output -xtags -wal pal $+ > $@
wal-using-heuristic.out: in/wal.Q.tags.tok in/wal.p.tok in/wal.p.pal
	markup_canoe_output -xtags -wal h $+ > $@
