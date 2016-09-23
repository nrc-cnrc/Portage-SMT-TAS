#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Validate all dependencies for Portage.
#
# @author Samuel Larkin, Eric Joanis, Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, 2016, Her Majesty in Right of Canada

-include Makefile.params

# Generic error message
EXPLAIN := Can not run your program.

# Stabilize the output
export PORTAGE_INTERNAL_CALL=1

SHELL = bash
.PHONY: all
all: testsuite

include ../Makefile.incl

.PHONY: testsuite
testsuite:
	@echo "Installation successful."


########################################
# What can be checked before starting to install Portage.
testsuite: pre_installation

.PHONY: pre_installation
pre_installation: gcc_version
pre_installation: perl_version perl_modules python_version python_modules java_version
pre_installation: make_version mgiza word2vec libsvm xml_grep xmllint ram_vs_swap
pre_installation:
	@echo "Prerequisites are correctly installed." >&2

.PHONY: perl_version
perl_version:
	@perl -e 'require 5.10.0;' || ! echo "Portage requires Perl 5.10 or greater for proper UTF-8 handling." >&2
	@perl -e 'use threads;' || ! echo "Perl must be installed with -Dusethreads." >&2

.PHONY: perl_modules
perl_modules:
	@rc=0; for module in XML::Twig XML::XPath JSON YAML File::Spec File::Temp List::Util Time::gmtime FindBin Env Time::HiRes; do \
		echo -n Perl module $$module: '' ; \
		if perldoc -l $$module > /dev/null; then echo OK; else echo NOT FOUND; rc=1; fi; \
	done; [[ $$rc == 0 ]] || ! echo "Some required Perl modules are missing." >&2
	@perl -e 'use XML::Twig 3.32;' || ! echo "Portage requires XML::Twig version 3.32 or greater." >&2

.PHONY: python_version
python_version:
	@python --version 2>&1 | grep -q 2\.7 || ! echo "Portage requires Python 2.7." >&2
	@ldd $$(which python) | egrep -q libpython2\.[0-9]+\.so \
	|| ! echo "Portage requires Python 2.7 built with dynamic library support (--enabled-shared). Refer to \$$PORTAGE/INSTALL." >&2

.PHONY: python_modules
python_modules:
	@rc=0; for module in argparse ctypes __future__ optparse os.path subprocess codecs glob gzip math os re sys string shlex shutil tempfile; do \
		echo -n Python module $$module: '' ; \
		if python -c "import $$module"; then echo OK; else echo NOT FOUND; rc=1; fi; \
		python -c "import $$module" || rc=1;\
	done; [[ $$rc == 0 ]] || ! echo "Some required Python modules are missing." >&2

.PHONY: java_version
java_version:
	@[[ $$(echo "$$(java -version 2>&1 | egrep -o -m 1 '[0-9]+\.[0-9]+') >= 1.6" | bc) == 1 ]]  \
	|| ! echo "Portage requires Java 1.6 or more recent." >&2

.PHONY: make_version
make_version:
	@[[ $$( echo "`\make --version | head -n 1 | egrep -o '[0-9\.]+'` >= 3.81" | bc) == 1 ]] \
	|| ! echo "Portage requires make-3.81" >&2


# Either this is a binary distribution of Portage in which case we don't need a compiler
# or we need to compile Portage with g++ version 4.2 or greater.
# In a binary distro, portage_info should be under something like:
# ~/PortageII-3.0/bin/x86_64-el5-icu/portage_info
.PHONY: gcc_version
gcc_version:
	@[[ `which portage_info` =~ bin/[^/]+/portage_info ]] \
	|| [[ $$( echo "`${CXX} --version | head -n 1 | egrep -om1 '[0-9]\.[0-9]+' | head -n 1` >= 4.2" | bc) == 1 ]] \
	|| ! echo "Portage requires at least g++-4.2, unless you have a binary distribution." >&2

.PHONY: mgiza
mgiza: EXPLAIN:=Make sure MGIZA is correctly installed.
mgiza:
	@which $@ &> /dev/null || ! echo "Portage requires MGIZA. Can't locate $@ in your PATH. Refer to \$$PORTAGE/INSTALL." >&2
	@mgiza --help 2>&1 | grep -q "Starting MGIZA" \
	|| ! echo "mgiza --help FAILED. ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2

.PHONY: word2vec
word2vec: EXPLAIN:=Make sure word2vec is correctly installed.
word2vec:
	@which $@ &> /dev/null || ! echo "Portage requires word2vec. Can't locate $@ in your PATH. Refer to \$$PORTAGE/INSTALL." >&2
	@word2vec 2>&1 | grep -q "WORD VECTOR estimation toolkit" \
	|| ! echo "word2vec FAILED. ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2

.PHONY: libsvm
libsvm: EXPLAIN:=Make sure libsvm is correctly installed.
libsvm:
	@which svm-predict &> /dev/null \
	|| ! echo "Portage requires libsvm. Can't locate svm-predict in your PATH. Refer to \$$PORTAGE/INSTALL." >&2
	@svm-predict 2>&1 | grep -q "svm-predict" \
	|| ! echo "svm-predict FAILED. ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2

.PHONY: xml_grep xmllint
xml_grep xmllint: EXPLAIN:=Make sure $@ is correctly installed.
xml_grep xmllint:
	@which $@ &> /dev/null \
	|| ! echo "Portage tmx2lfl.pl requires $@. Can't locate $@ in your PATH. Refer to \$$PORTAGE/INSTALL." >&2
	@$@ --version 2>&1 | grep -q "$@" \
	|| ! echo "$@ --version FAILED. ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2

# Since the ram is power of 1024 but swap is power of 1000, we will make the
# conversion here.
.PHONY: ram_vs_swap
ram_vs_swap:
	@SWAP=`cat /proc/meminfo | grep SwapTotal | egrep -o '[0-9]+'`; \
	RAM=`cat /proc/meminfo | grep MemTotal | egrep -o '[0-9]+'`; \
	[[ $$SWAP -ge $$((2 * $$RAM * 1000 / 1024 * 1000 / 1024)) ]] \
	|| echo "Warning: We strongly suggest that you have twice as much swap than ram." >&2


########################################
# What needs to work after a successful installation of Portage.
testsuite: post_installation

VALIDATE_PGMS := canoe tune.py utokenize.pl ce.pl tmx2lfl.pl filter-nc1.py casemark.py arpalm.encode
.PHONY: post_installation
post_installation: ${VALIDATE_PGMS}
	@echo "Portage itself is correctly installed." >&2

canoe: EXPLAIN:=Make sure Portage is correctly installed.
ce.pl: EXPLAIN:=Check your PERL5LIB or XML::Twig may not be correctly installed.
tune.py: EXPLAIN:=Check your Python version. Make sure Portage is correctly installed.
tmx2lfl.pl: EXPLAIN:=Check your PERL5LIB, or you're missing XML::Twig, or your XML::Twig version is less than 3.32.
utokenize.pl: EXPLAIN:=Check your PERL5LIB.
filter-nc1.py: EXPLAIN:=Check your Python version and PYTHONPATH.
casemark.py: EXPLAIN:=Check your Python version and PYTHONPATH. Make sure Python is installed correctly (with dynamic library support).
arpalm.encode: EXPLAIN:=Check that all boost libraries are installed.

.PHONY: ${VALIDATE_PGMS}
# We simply make sure we can invoke the help message to trigger the dependencies.
${VALIDATE_PGMS}:
	@which $@ &> /dev/null || ! echo "Can't locate $@ in your PATH. Refer to \$$PORTAGE/INSTALL." >&2
	@$@ -h 2>&1 | head -5 | diff - ref/$@.ref || ! echo "$@ -h FAILED: ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2

post_installation: check_casemark

.PHONY: check_casemark
check_casemark: EXPLAIN:=Check your LD_LIBRARY_PATH: it should include the location of libportage_truecasing.so. \
                         Check your PYTHONPATH: it should include the location of portage_utils.pyc. \
                         Make sure Python is installed correctly (with dynamic library support).
check_casemark:
	@casemark.py -lm /dev/null /dev/null 2>&1 | diff - ref/casemark.py.error-ref \
	|| ! echo "casemark.py -lm /dev/null /dev/null FAILED: ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2

post_installation: check_structpred

.PHONY: check_structpred
check_structpred: EXPLAIN:=Make sure Java is correctly installed.
check_structpred:
	@which structpred.jar &> /dev/null || ! echo "Can't locate structpred.jar in your PATH. Refer to \$$PORTAGE/INSTALL." >&2
	@java -jar `which structpred.jar` JavaVersion 2>&1 | grep -q "StructPred version" \
	|| ! echo "structpred.jar JavaVersion FAILED: ${EXPLAIN} Refer to \$$PORTAGE/INSTALL." >&2
