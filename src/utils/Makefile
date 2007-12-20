# Makefile for Portage utils

# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

OBJECTS = \
        arg_reader.o \
        bc_stats.o \
        docid.o \
        errors.o \
        file_utils.o \
        gfmath.o \
        gfstats.o \
        good_turing.o \
        histogram_fit.o \
        ls_poly_fit.o \
        MagicStream.o \
        matrix_solver.o \
        multi_voc.o \
        parse_xmlish_markup.o \
        perm_iterator.o \
        portage_defs.o \
        quick_set.o \
        str_utils.o \
        trie.o \
        voc.o \
        vocab_filter.o \
        voc_map.o

LIBRARY = libportage_utils

TESTPROGS =  \
        prog \
        test_MagicStream \
        test_trie \
        tester \
        testleven \
        try

PROGRAMS = $(TESTPROGS) \
        bin_class_stats \
        bloater \
        get_voc \
        good_turing_estm \
        iroc_exp \
        meanvar \
        sample_parallel_text


BINSCRIPTS = \
        alignment-stats.pl \
        append-uniq.pl \
        canoe2rescoreFile.sh \
        combine.pl \
        cvs-status-summary.pl \
        diff-round.pl \
        filter-for-giza.pl \
        filter-latin.pl \
        getcolumn.sh \
        lc-latin.pl \
        load-balancing.pl \
        monitor-process.sh \
        mteval2ospl.pl \
        mteval2ospl.sed \
        ngrams.pl \
        ospl2mteval.pl \
        outpipe.sh \
        parallel-uniq.pl \
        parse-monitor.sh \
        psub \
        randomly-choose-n-args.pl \
        remove_stop.pl \
        removewords.pl \
        reverseffvals.pl \
        run-after.sh \
        run-parallel.sh \
        run-parallel-d.pl \
        sort-by-length.pl \
        split-listed.pl \
        stableuniq.pl \
        stableuniqffvals.pl \
        strip-markup.pl \
        sum.pl \
        trans-search.pl \
        uc-first.pl \
        uncombine.pl \
        which-test.sh \
        worker.pl

INFOFILES =

EXTRA_LIBS =

# Tell Make to skip -lportage_utils, since we're building it here.
NO_UTILS_LIB = true

DOCUMENTS =

# Module/directory specific compile flags
MODULE_CF=

include ../build/Makefile.incl
-include Makefile.depend
