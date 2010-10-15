# Makefile for Portage utils

# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
        arg_reader.o \
        bc_stats.o \
        casemap_strings.o \
        compact_phrase.o \
        em.o \
        errors.o \
        file_utils.o \
        gfmath.o \
        gfstats.o \
        good_turing.o \
        histogram_fit.o \
        hmm.o \
        ls_poly_fit.o \
        MagicStream.o \
        matrix_solver.o \
        multi_voc.o \
        number_mapper.o \
        parse_xmlish_markup.o \
        perm_iterator.o \
        portage_defs.o \
        process_bind.o \
        quick_set.o \
        str_utils.o \
        trie.o \
        utf8_utils.o \
        voc.o \
        voc_map.o \
        vocab_filter.o \
        word_classes.o

LIBRARY = libportage_utils

TESTPROGS =  \
        prog \
        test_MagicStream \
        testleven \
        try

PROGRAMS = $(TESTPROGS) \
        bin_class_stats \
        bloater \
        bloater_pthread \
        good_turing_estm \
        iroc_exp \
        meanvar \
        merge_counts \
        portage_info \
        wc_stats

BINSCRIPTS = \
        alignment-stats.pl \
        all-best-rr-vals \
        best-rr-val \
        cvs-status-summary.pl \
        diff-round.pl \
        load-balancing.pl \
        monitor-process.sh \
        mteval2ospl.pl \
        mteval2ospl.sed \
        ospl2mteval.pl \
        parallelize.pl \
        parse-monitor.pl \
        plog.pl \
        process-bind.sh \
        process-memory-usage.pl \
        psub \
        randomly-choose-n-args.pl \
        randomly-choose-n-nums \
        reverseffvals.pl \
        run-after.sh \
        run-parallel.sh \
        r-parallel-d.pl \
        r-parallel-worker.pl \
        sh_utils.sh \
        sum.pl \
        time-mem \
        time-mem-tally.pl \
        trans-search.pl \
        which-test.sh

PGM_WITH_ALTERNATIVE_HELP = plog.pl

PERL_LIBS=portage_utils.pm

# Tell Make to skip -lportage_utils, since we're building it here.
NO_UTILS_LIB = true

DOCUMENTS =

# Module/directory specific compile flags
MODULE_CF=

include ../build/Makefile.incl

# Special rule to run legacy_test_trie, because it requires an input data file
legacy_test_trie: tests/legacy/test_trie tests/data/test_trie.in.txt \
                  tests/data/test_trie.out.txt
	tests/legacy/test_trie < tests/data/test_trie.in.txt | \
           diff tests/data/test_trie.out.txt -
	${RM} test_trie.binary

# Some programs need extra dynamic libraries
bloater: DYNLIBS += -lpthread
bloater_pthread: DYNLIBS += -lpthread
tests/test_casemap_strings: DYNLIBS += $(LIBICU)

# Additional tests to make sure programs we need are present
test: check_make381

.PHONY: check_make381
check_make381:
	@perl -e 'if ( '`make --version | egrep -o '[0-9]+\.[0-9]+'`' < 3.81 ) { print "Portage requires GNU make 3.81\n"; exit 1; }'

