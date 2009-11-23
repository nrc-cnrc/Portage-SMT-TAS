# Makefile - Test the word classes HMM training regime
# vim:noet
#
# Usage:
#   make tm-hmm
#     builds a bunch of models with varying parameter sets.
#   make tm-hmm CORPUS=dev1
#     builds a bunch of models with varying parameter sets on a tiny corpus
#   make test
#     runs one-step regression testing
#
# Eric Joanis, based on George Foster's toy example
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

all: test

.SECONDARY:

DIFF_IGNORE = --ignore-blank-lines --ignore-matching-lines=TTable --ignore-matching-lines=NRC-CNRC

clean:
	rm -f ibm[12]* phrases* hmm-*
	rm -f log.*
	rm -rf tmp

.PHONY: run-test1 test1 run-test2 test2 run-test3 test3 \
        run-test4 test4 run-test5 test5 run-test6 test6 \
        run-test7 test7 run-test8 test8 run-test9 test9 \
        run-test10 test10 \
        test

test: test1 test2 test3 test4 test5 test6 test7 test8 test9 test10
	@echo
	@echo All tests PASSED

run-test1:
	rm -f hmm-base-dev1.fr_given_en{,.dist}
	train_ibm -v -n1 0 -i ref/ibm1-dev1.fr_given_en \
	   -hmm -up0 0.1 -p0 0.05 \
	   hmm-base-dev1.fr_given_en ref/dev1_en.al ref/dev1_fr.al \
	   &> log.hmm-base-dev1.fr_given_en

test1: run-test1
	diff-round.pl -p 5 -sort ref/hmm-base-dev1.fr_given_en.gz hmm-base-dev1.fr_given_en -q && \
	diff-round.pl -p 5 -sort ref/hmm-base-dev1.fr_given_en.dist hmm-base-dev1.fr_given_en.dist -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.hmm-base-dev1.fr_given_en | \
	   diff $(DIFF_IGNORE) ref/log.hmm-base-dev1.fr_given_en - -q

run-test2:
	rm -f hmm-base-dev1.en_given_fr{,.dist}
	train_ibm -v -n1 0 -i ref/ibm1-dev1.en_given_fr \
	   -hmm -up0 0.1 -p0 0.05 \
	   hmm-base-dev1.en_given_fr ref/dev1_fr.al ref/dev1_en.al \
	   &> log.hmm-base-dev1.en_given_fr

test2: run-test2
	diff-round.pl -sort ref/hmm-base-dev1.en_given_fr.gz hmm-base-dev1.en_given_fr -q && \
	diff-round.pl -sort ref/hmm-base-dev1.en_given_fr.dist hmm-base-dev1.en_given_fr.dist -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.hmm-base-dev1.en_given_fr | \
	   diff $(DIFF_IGNORE) ref/log.hmm-base-dev1.en_given_fr - -q

run-test3:
	gen_phrase_tables -1 fr -2 en -o phrases-hmm-base-dev1 \
	   -multipr fwd -v -s RFSmoother -s IBMSmoother \
	   ref/hmm-base-dev1.en_given_fr ref/hmm-base-dev1.fr_given_en \
	   ref/dev1_fr.al ref/dev1_en.al \
	   &> log.phrases-hmm-base-dev1.fr2en

test3: run-test3
	diff-round.pl -sort ref/phrases-hmm-base-dev1.fr2en.gz phrases-hmm-base-dev1.fr2en -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.phrases-hmm-base-dev1.fr2en | \
	   diff-round.pl -p 6 ref/log.phrases-hmm-base-dev1.fr2en.gz - -q

run-test4:
	rm -f hmm-liang-dev1.fr_given_en hmm-liang-dev1.fr_given_en.dist
	train_ibm -v -n1 0 -i ref/ibm1-dev1.fr_given_en \
	   -hmm -liang hmm-liang-dev1.fr_given_en ref/dev1_en.al ref/dev1_fr.al \
	   &> log.hmm-liang-dev1.fr_given_en

test4: run-test4
	diff-round.pl -p 5 -sort ref/hmm-liang-dev1.fr_given_en.gz hmm-liang-dev1.fr_given_en -q && \
	diff-round.pl -p 5 -sort ref/hmm-liang-dev1.fr_given_en.dist hmm-liang-dev1.fr_given_en.dist -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.hmm-liang-dev1.fr_given_en | \
	   diff $(DIFF_IGNORE) ref/log.hmm-liang-dev1.fr_given_en - -q

run-test5:
	rm -f hmm-liang-dev1.en_given_fr hmm-liang-dev1.en_given_fr.dist
	train_ibm -v -n1 0 -i ref/ibm1-dev1.en_given_fr \
	   -hmm -liang hmm-liang-dev1.en_given_fr ref/dev1_fr.al ref/dev1_en.al \
	   &> log.hmm-liang-dev1.en_given_fr

test5: run-test5
	diff-round.pl -p 5 -sort ref/hmm-liang-dev1.en_given_fr.gz hmm-liang-dev1.en_given_fr -q && \
	diff-round.pl -p 5 -sort ref/hmm-liang-dev1.en_given_fr.dist hmm-liang-dev1.en_given_fr.dist -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.hmm-liang-dev1.en_given_fr | \
	   diff $(DIFF_IGNORE) ref/log.hmm-liang-dev1.en_given_fr - -q

run-test6:
	gen_phrase_tables -1 fr -2 en -o phrases-hmm-liang-dev1 \
	   -multipr fwd -v -s RFSmoother -s IBMSmoother \
	   ref/hmm-liang-dev1.en_given_fr ref/hmm-liang-dev1.fr_given_en \
	   ref/dev1_fr.al ref/dev1_en.al \
	   &> log.phrases-hmm-liang-dev1.fr2en

test6: run-test6
	diff-round.pl -sort -p 4 ref/phrases-hmm-liang-dev1.fr2en.gz phrases-hmm-liang-dev1.fr2en -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.phrases-hmm-liang-dev1.fr2en | \
	   diff-round.pl -p 6 ref/log.phrases-hmm-liang-dev1.fr2en.gz - -q

run-test7:
	rm -f hmm-heb-dev1.fr_given_en hmm-heb-dev1.fr_given_en.dist
	train_ibm -v -n1 0 -i ref/ibm1-dev1.fr_given_en \
	   -hmm -mimic he-baseline -word-classes-l1 ref/dev1_en.cls \
	   hmm-heb-dev1.fr_given_en ref/dev1_en.al ref/dev1_fr.al \
	   &> log.hmm-heb-dev1.fr_given_en

test7: run-test7
	diff-round.pl -p 5 -sort ref/hmm-heb-dev1.fr_given_en.gz hmm-heb-dev1.fr_given_en -q && \
	diff-round.pl -p 5 -sort ref/hmm-heb-dev1.fr_given_en.dist hmm-heb-dev1.fr_given_en.dist -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.hmm-heb-dev1.fr_given_en | \
	   diff $(DIFF_IGNORE) ref/log.hmm-heb-dev1.fr_given_en - -q

run-test8:
	rm -f hmm-heb-dev1.en_given_fr hmm-heb-dev1.en_given_fr.dist
	train_ibm -v -n1 0 -i ref/ibm1-dev1.en_given_fr \
	   -hmm -mimic he-baseline -word-classes-l1 ref/dev1_fr.cls \
	   hmm-heb-dev1.en_given_fr ref/dev1_fr.al ref/dev1_en.al \
	   &> log.hmm-heb-dev1.en_given_fr

test8: run-test8
	diff-round.pl -p 5 -sort ref/hmm-heb-dev1.en_given_fr.gz hmm-heb-dev1.en_given_fr -q && \
	diff-round.pl -p 5 -sort ref/hmm-heb-dev1.en_given_fr.dist hmm-heb-dev1.en_given_fr.dist -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.hmm-heb-dev1.en_given_fr | \
	   diff $(DIFF_IGNORE) ref/log.hmm-heb-dev1.en_given_fr - -q

run-test9:
	gen_phrase_tables -1 fr -2 en -o phrases-hmm-heb-dev1 \
	   -multipr fwd -v -s RFSmoother -s IBMSmoother \
	   ref/hmm-heb-dev1.en_given_fr ref/hmm-heb-dev1.fr_given_en \
	   ref/dev1_fr.al ref/dev1_en.al \
	   &> log.phrases-hmm-heb-dev1.fr2en

test9: run-test9
	diff-round.pl -p 5 -sort ref/phrases-hmm-heb-dev1.fr2en.gz phrases-hmm-heb-dev1.fr2en -q && \
	sed 's/ [0-9][0-9]* second/ N second/' < log.phrases-hmm-heb-dev1.fr2en | \
	   diff-round.pl -p 6 ref/log.phrases-hmm-heb-dev1.fr2en.gz - -q

run-test10:
	rm -f hmm-heb-cat-dev1.fr_given_en hmm-heb-cat-dev1.fr_given_en.dist
	train_ibm -v -n1 0 -i ref/ibm1-dev1.fr_given_en \
	   -hmm -mimic he-baseline -word-classes-l1 ref/dev1_en.cls \
	   hmm-heb-cat-dev1.fr_given_en ref/dev1_en.al ref/dev1_fr.al \
	   &> log.hmm-heb-cat-dev1.fr_given_en

test10: run-test10 run-test7
	diff-round.pl -p 4 hmm-heb-dev1.fr_given_en hmm-heb-cat-dev1.fr_given_en -q && \
	diff-round.pl -p 5 -sort hmm-heb-dev1.fr_given_en.dist hmm-heb-cat-dev1.fr_given_en.dist -q
