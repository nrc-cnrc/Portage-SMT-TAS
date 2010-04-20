# Makefile for confidence estimation
# $Id$
#
# Michel Simard
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

BINSCRIPTS = \
        ce_canoe2ffvals.pl \
        ce_ttx2ospl.pl \
        ce.pl \
        ce_train.pl \
        ce_translate.pl \
        ce_gen_features.pl \
        ce_tmx.pl

# See PERL_LIB_DIR line at end of Makefile
PERL_LIBS = \
        CE/base.pm \
        CE/data.pm \
        CE/dataset.pm \
        CE/distance.pm \
        CE/feature.pm \
        CE/libsvm.pm \
        CE/model.pm \
        CE/normalize.pm \
        CE/help.pm

test: check_perl_dependencies

check_perl_dependencies:
	perldoc -l File::Spec
	perldoc -l File::Temp
	perldoc -l List::Util
	perldoc -l Time::gmtime
	perldoc -l XML::Twig

PGM_WITH_ALTERNATIVE_HELP = ce.pl

include ../build/Makefile.incl

# Ugly Hack: Local override of PERL_LIB_DIR to make sure Perl Packages
# get installed as a directory:
PERL_LIB_DIR := $(PERL_LIB_DIR)/CE
