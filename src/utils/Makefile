# Makefile for Portage utils

# George Foster
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

SOURCES = \
        arg_reader.cc \
        bc_stats.cc \
        bin_class_stats.cc \
        errors.cc \
        file_utils.cc \
        get_voc.cc \
        gfmath.cc \
        good_turing.cc \
        good_turing_estm.cc \
        histogram_fit.cc \
        iroc_exp.cc \
        ls_poly_fit.cc \
        MagicStream.cc \
        matrix_solver.cc \
        meanvar.cc \
        perm_iterator.cc \
        portage_defs.cc \
        prog.cc \
        quick_set.cc \
        sample_parallel_text.cc \
        str_utils.cc \
        test_MagicStream.cc \
        test_trie.cc \
        testleven.cc \
        try.cc \
        voc.cc \
        voc_map.cc

INCLUDES = \
        arg_reader.h \
        array_mem_pool.h \
        array_mem_pool-cc.h \
        bc_stats.h \
        block_mem_pool.h \
        bootstrap.h \
        errors.h \
        exception_dump.h \
        file_utils.h \
        gfstats.h \
        gfmath.h \
        good_turing.h \
        histogram_fit.h \
        index_block_mem_pool.h \
        levenshtein.h \
        ls_poly_fit.h \
        MagicStream.h \
        matrix_solver.h \
        parse_xmlish_markup.h \
        perm_iterator.h \
        portage_defs.h \
        progress.h \
        quick_set.h \
        str_utils.h \
        trie.h \
        trie-cc.h \
        vector_map.h \
        vcb.h \
        voc.h \
        voc_map.h

OBJECTS = \
        arg_reader.o \
        bc_stats.o \
        errors.o \
        file_utils.o \
        gfmath.o \
        good_turing.o \
        histogram_fit.o \
        ls_poly_fit.o \
        MagicStream.o \
        matrix_solver.o \
        parse_xmlish_markup.o \
        perm_iterator.o \
        portage_defs.o \
        quick_set.o \
        str_utils.o \
        voc.o \
        voc_map.o

LIBRARY = libportage_utils

PROGRAMS = \
        bin_class_stats \
        bloater \
        get_voc \
        good_turing_estm \
        iroc_exp \
        meanvar \
        prog \
        sample_parallel_text \
        test_MagicStream \
        test_trie \
        testleven \
        tester \
        try

TESTPROGS =  bloater prog try tester test_trie test_MagicStream testleven

BINSCRIPTS = \
        alignment-stats.pl \
        append-uniq.pl \
        canoe2rescoreFile.sh \
        combine.pl \
        cvs-status-summary.pl \
        diff-round.pl \
        faucet.pl \
        faucet_launcher.sh \
        filter-for-giza.pl \
        filter-latin.pl \
        getcolumn.sh \
        lc-latin.pl \
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

DOXYFILE = ../build/Doxyfile

include ../build/Makefile.incl
-include Makefile.depend
