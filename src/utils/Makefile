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
        colours.o \
        compact_phrase.o \
        em.o \
        errors.o \
        file_utils.o \
        gfmath.o \
        gfstats.o \
        good_turing.o \
        histogram_fit.o \
        hmm.o \
        line_indexed_file.o \
        ls_poly_fit.o \
        MagicStream.o \
        matrix_solver.o \
        mm_map.o \
        multi_voc.o \
        ngram_counts.o \
        number_mapper.o \
        parse_xmlish_markup.o \
        perm_iterator.o \
        portage_defs.o \
        process_bind.o \
        quick_set.o \
        socket_utils.o \
        stats.o \
        str_utils.o \
        timer.o \
        tm_entry.o \
        toJSON.o \
        trie.o \
        utf8_utils.o \
        voc.o \
        voc_map.o \
        vocab_filter.o \
        wordClassMapper.o \
        word_classes.o

LIBRARY = libportage_utils

TESTPROGS =  \
        prog \
        test_MagicStream \
        test_huffman \
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
        merge_multi_column_counts \
        portage_info \
        wc_stats \
        wordClasses2MMmap

BINSCRIPTS = \
        alignment-stats.pl \
        all-best-rr-vals \
        best-rr-val \
        fast_align_glue_corpora.py \
        fast_align_normalize_ttable.py \
        incr-add-sentence.sh \
        incr-init-model.py \
        incr-update.sh \
        load-balancing.pl \
        monitor-process.sh \
        parse-monitor.pl \
        plive-optimize-pretrained.sh \
        plog.pl \
        prep-fixedTerms-layout.sh \
        process-bind.sh \
        randomly-choose-n-args.pl \
        randomly-choose-n-nums \
        reverseffvals.pl \
        run-after.sh \
        run-mkcls.sh \
        sh_utils.sh \
        summarize-canoe-results.py \
        time-mem \
        time-mem-tally.pl \
        trans-search.pl

PGM_WITH_ALTERNATIVE_HELP = plog.pl

PERL_LIBS=portage_utils.pm

# Install the .py file not the .pyc to allow Python (2.7 or 3.X) being used to
# compile the file and drop it in the appropriate place.
PYTHON_LIBS=portage_utils.py

# Tell Make to skip -lportage_utils, since we're building it here.
NO_UTILS_LIB = true

DOCUMENTS =

IGNORES = tests/test_*.mmMap tests/test_nnjm_word2class.MMmap tests/test_nnjm_word2class.txt

# Module/directory specific compile flags
MODULE_CF=

ifeq ($(shell bash -c ulimit -v),unlimited)
tests/test_MagicStream: MODULE_CF += -DUNLIMITED_VMEM
endif

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
tests/test_utf8_utils: DYNLIBS += $(LIBICU)
wc_stats:  OPTS += -fopenmp

# Additional tests to make sure programs we need are present
test: check_make381

.PHONY: check_make381
check_make381:
	@perl -e 'if ( '`make --version | egrep -o -m 1 '[0-9]+\.[0-9]+'`' < 3.81 ) { print "Portage requires GNU make 3.81\n"; exit 1; }'

# jobtree has some Perl dependencies
test: check_perl_dependencies

# Term::ANSIColor 3.00 is needed for the bright_ colours; otherwise Term::ANSIColor 2.02 is needed for colorvalid.
# Note: we make the Term::ANSIColor 3.00, Data::TreeDumper and Tree::Simple tests issue only
# a warning because these are only for jobtree and therefore only for clusters.
.PHONY: check_perl_dependencies
check_perl_dependencies:
	-perldoc -l Data::TreeDumper
	-perldoc -l Tree::Simple
	perldoc -l Time::Piece
	-perl -e 'use Term::ANSIColor 3.00;'

