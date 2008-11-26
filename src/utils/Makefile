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
        docid.o \
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
        get_voc \
        good_turing_estm \
        iroc_exp \
        meanvar \
        merge_counts \
        sample_parallel_text \
        utf8_casemap \
        utf8_filter


BINSCRIPTS = \
        alignment-stats.pl \
        all-best-rr-vals \
        append-uniq.pl \
        best-rr-val \
        canoe2rescoreFile.sh \
        combine.pl \
        cvs-status-summary.pl \
        diff-round.pl \
        filter-for-giza.pl \
        filter-latin.pl \
        getcolumn.sh \
        lc-latin.pl \
        li-sort.sh \
        load-balancing.pl \
        monitor-process.sh \
        mteval2ospl.pl \
        mteval2ospl.sed \
        ngrams.pl \
        ospl2mteval.pl \
        outpipe.sh \
        parallel-uniq.pl \
        parse-monitor.sh \
        process-memory-usage.pl \
        psub \
        randomly-choose-n-args.pl \
        remove_stop.pl \
        removewords.pl \
        reverseffvals.pl \
        run-after.sh \
        run-parallel.sh \
        r-parallel-d.pl \
        r-parallel-worker.pl \
        select-line \
        sort-by-length.pl \
        split-listed.pl \
        stableuniq.pl \
        stableuniqffvals.pl \
        sum.pl \
        trans-search.pl \
        uc-first.pl \
        uncombine.pl \
        which-test.sh

# Tell Make to skip -lportage_utils, since we're building it here.
NO_UTILS_LIB = true

DOCUMENTS =

# Module/directory specific compile flags
MODULE_CF=

include ../build/Makefile.incl
-include Makefile.depend

# Special rule to run legacy_test_trie, because it requires an input data file
legacy_test_trie: tests/legacy/test_trie tests/data/test_trie.in.txt \
                  tests/data/test_trie.out.txt
	tests/legacy/test_trie < tests/data/test_trie.in.txt | \
           diff tests/data/test_trie.out.txt -

# Some programs need extra dynamic libraries
utf8_casemap: DYNLIBS += $(LIBICU)
