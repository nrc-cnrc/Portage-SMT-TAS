#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Unittest for script feature function.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada



-include Makefile.params


.PHONY: all
all: rescore


TEMP_FILES=log.rat log.rescored.test rescore-model rescore_ON_test_fr.out run-parallel-logs* \
           workdir-dev2_fr.al-100best/ff.SCRIPT* \
           workdir-dev2_fr.al-100best/mon.run_cmd.* \
           workdir-dev2_fr.al-100best/*.old?? \
           workdir-dev2_fr.al-100best/rescore-model.ini.rat* \
           workdir-test_fr.al-100best/ff.SCRIPT* \
           workdir-test_fr.al-100best/mon.run_cmd.* \
           workdir-test_fr.al-100best/*.old?? \
           workdir-test_fr.al-100best/rescore-model.rat \
           gen-features-*.commands
TEMP_DIRS=run-p.[0-9]*
include ../Makefile.incl


rat: rescore-model
rescore-model: canoe.ini.cow rescore-model.ini
	@echo "Training a rescoring model"
	rat.sh \
		-no-filt \
		-n 2 train -dep -v -d -n 2 \
		-f $< \
		-K 100 \
		-o $@ \
		rescore-model.ini dev2_fr.al dev2_en.al &> log.rat
	zcat workdir-dev2_fr.al-100best/ff.SCRIPT.zcatworkdir-dev2_fr.al-100best_100best.gz_perl-nle#print2#.gz | perl -nle 'exit 1 unless($$_ == "2")'
	[[ `zcat workdir-dev2_fr.al-100best/ff.SCRIPT.zcatworkdir-dev2_fr.al-100best_100best.gz_perl-nle#print2#.gz | \wc -l` == 10000 ]]
	zcat workdir-dev2_fr.al-100best/ff.SCRIPT.stub-feature-function.shworkdir-dev2_fr.al-100best_100best.gz-nN.gz | perl -nle 'exit 1 unless($$_ == "2")'
	[[ `zcat workdir-dev2_fr.al-100best/ff.SCRIPT.stub-feature-function.shworkdir-dev2_fr.al-100best_100best.gz-nN.gz | \wc -l` == 10000 ]]


rescore: rescore_ON_test_fr.out
rescore_ON_test_fr.out: rescore-model
	@echo "Rescoring test set"
	rat.sh -no-filt -n 2 trans -dep -v -n 2 -f canoe.ini.cow -K 100 $< test_fr.al \
	&> log.rescored.test
	mv test_fr.al.rat $@
	zcat workdir-test_fr.al-100best/ff.SCRIPT.zcatworkdir-test_fr.al-100best_100best.gz_perl-nle#print2#.gz | perl -nle 'exit 1 unless($$_ == "2")'
	[[ `zcat workdir-test_fr.al-100best/ff.SCRIPT.zcatworkdir-test_fr.al-100best_100best.gz_perl-nle#print2#.gz | \wc -l` == 10000 ]]
	zcat workdir-test_fr.al-100best/ff.SCRIPT.stub-feature-function.shworkdir-test_fr.al-100best_100best.gz-nN.gz | perl -nle 'exit 1 unless($$_ == "2")'
	[[ `zcat workdir-test_fr.al-100best/ff.SCRIPT.stub-feature-function.shworkdir-test_fr.al-100best_100best.gz-nN.gz | \wc -l` == 10000 ]]

