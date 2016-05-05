# Makefile for basic textutils
#
# @author Eric Joanis
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

PROGRAMS = \
	asciify \
	filter_word_classes \
	get_voc \
	strip_non_printing \
	utf8_casemap \
	utf8_filter \
	word2class

# NOTE: DO NOT add the detokenize_plugin script to the BINSCRIPTS variable since
# it exists only to illustrate what a detokenie_plugin could look like.
BINSCRIPTS = $(wildcard *.sh *.pl *.py) \
	preprocess_plugin \
	predecode_plugin \
	postdecode_plugin \
	postprocess_plugin \
	tokenize_plugin \
	tokenize_plugin_ar \
	sentsplit_plugin \
	select-line

test: check_perl_dependencies

check_perl_dependencies:
	perldoc -l FindBin

include ../build/Makefile.incl

# Custom install target for tokenize Arabic Data Perl module.
install_perl_lib:   $(PERL_LIB_DIR)/tokenize/Arabic/Data.pm

$(PERL_LIB_DIR)/tokenize/Arabic/Data.pm:  tokenize/Arabic/Data.pm
	install -d -m0750 $(dir $@)
	install -p -m0640 $< $(dir $@)


# Some programs need extra dynamic libraries
utf8_casemap: DYNLIBS += $(LIBICU)


test:  perl_doctest

.PHONY:  perl_doctest
perl_doctest:  $(addsuffix .doc_test, $(wildcard *.pm))
%.doc_test:  %
	perl -MTest::Doctest -e run $<
