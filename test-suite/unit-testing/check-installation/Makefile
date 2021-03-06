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

# Generic error messages
EXPLAIN := Cannot run your program.
EXPLAININSTALL = ${EXPLAIN} Refer to \$$PORTAGE/INSTALL.

# Stabilize the output
export PORTAGE_INTERNAL_CALL=1

SHELL = bash
.PHONY: all
all: testsuite

include ../Makefile.incl

.PHONY: testsuite
testsuite:
	@echo $$'Installation successful.\n'

# Function to run a test with a given summary label, and a given error text if it fails
#DEBUG_RUN=1
ifdef DEBUG_RUN
   check = echo "<D>$2"; echo -n "$1: "; if $2; then echo OK; else echo "***" $3 "***"; false; fi
else
   check = echo -n "$1: "; if $2; then echo OK; else echo "***" $3 "***"; false; fi
endif

check_installed = $(call check,$1 installed,which $1 >& /dev/null,NOT FOUND. ${EXPLAININSTALL})

foo:
	@$(call check,run true,true,ERROR)
	@$(call check,run false,false,ERROR)

########################################
# What can be checked before starting to install Portage.
testsuite: pre_installation

.PHONY: pre_installation
pre_installation: gcc_version
pre_installation: perl_version perl_modules java_version
pre_installation: python3_version python3_modules python2_version python2_modules
pre_installation: make_version mgiza word2vec libsvm xml_grep xmllint ram_vs_swap
pre_installation:
	@echo $$'\nPrerequisites are correctly installed.\n'

.PHONY: perl_version
perl_version:
	@$(call check,Perl version,perl -e 'require 5.14.0;',Portage requires Perl 5.14 or greater for proper UTF-8 handling.)
	@$(call check,Perl -Dusethreads,perl -e 'use threads',Perl must be installed with -Dusethreads.)

.PHONY: perl_modules
perl_modules:
	@rc=0; for module in XML::Twig XML::XPath XML::Writer JSON YAML File::Spec File::Temp List::Util Time::gmtime FindBin Env Time::HiRes; do \
		$(call check,Perl module $$module,perldoc -l $$module >& /dev/null,NOT FOUND) || rc=1; \
	done; [[ $$rc == 0 ]] || ! echo "Some required Perl modules are missing."
	@$(call check,XML::Twig version,perl -e 'use XML::Twig 3.32;',Portage requires XML::Twig version 3.32 or greater.)

.PHONY: python2_version
python2_version:
	@$(call check,Python 2.7,python2 --version 2>&1 | grep -q 2\.7,Portage requires Python 2.7)
	@$(call check,Python 2.7 with dynamic library,\
		ldd $$(which python2) | egrep -q libpython2\.[0-9]+\.so,\
		"Portage requires Python 2.7 built with dynamic library support (--enabled-shared). Refer to \$$PORTAGE/INSTALL.")

.PHONY: python2_modules
python2_modules:
	@rc=0; for module in argparse ctypes __future__ optparse os.path subprocess codecs glob gzip math os re sys string shlex shutil tempfile; do \
		$(call check,Python 2.7 module $$module,python2 -c "import $$module" >& /dev/null,NOT FOUND) || rc=1; \
	done; [[ $$rc == 0 ]] || ! echo "Some required Python 2.7 modules are missing."

.PHONY: python3_version
python3_version:
	@$(call check,Python 3,python3 --version >& /dev/null,Portage requires Python 3 to be available)

.PHONY: python3_modules
python3_modules:
	@rc=0; for module in click regex; do \
		$(call check,Python 3 module $$module,python3 -c "import $$module" >& /dev/null,NOT FOUND) || rc=1; \
	done; [[ $$rc == 0 ]] || ! echo "Some required Python 3 modules are missing."

.PHONY: java_version
java_version:
	@$(call check,Java version,\
		[[ $$(echo "$$(java -Xmx1000m -version 2>&1 | egrep -o -m 1 '[0-9]+\.[0-9]+') >= 1.8" | bc) == 1 ]],\
		"Portage requires Java 1.8 or more recent." Found `which java`: `java -Xmx1000m -version 2>&1`.)

.PHONY: make_version
make_version:
	@$(call check,make version,\
		[[ $$( echo "`\make --version | head -n 1 | egrep -o '[0-9]\.[0-9]+'` >= 3.81" | bc) == 1 ]],\
		Portage requires make-3.81)


# Either this is a binary distribution of Portage in which case we don't need a compiler
# or we need to compile Portage with g++ version 4.2 or greater.
# If portage_info is found on the path, we'll assume we have compiled or pre-compiled
# Portage and we don't need to check the compiler version.
.PHONY: gcc_version
gcc_version:
	@[[ `which portage_info` =~ bin.*/portage_info ]] \
	|| [[ $$( echo "`${CXX} --version | head -n 1 | egrep -om1 '[0-9]\.[0-9]+' | head -n 1` >= 4.2" | bc) == 1 ]] \
	|| ! echo "*** Portage requires at least g++-4.2, unless you have a binary distribution. ***"

.PHONY: mgiza
mgiza: EXPLAIN:=Make sure MGIZA is correctly installed.
mgiza:
	@$(call check_installed,$@)
	@$(call check,$@ runs,\
		$@ --help 2>&1 | grep -q "Starting MGIZA",\
		"mgiza --help FAILED. ${EXPLAININSTALL}")

.PHONY: word2vec
word2vec: EXPLAIN:=Make sure word2vec is correctly installed.
word2vec:
	@$(call check_installed,$@)
	@$(call check,$@ runs,\
		$@ 2>&1 | grep -q "WORD VECTOR estimation toolkit",\
		"FAILED. ${EXPLAININSTALL}")

.PHONY: libsvm
libsvm: EXPLAIN:=Make sure libsvm is correctly installed.
libsvm:
	@$(call check_installed,svm-predict)
	@$(call check,svm-predict runs,\
		svm-predict 2>&1 | grep -q "Usage: svm-predict",\
		"FAILED. ${EXPLAININSTALL}")

.PHONY: xml_grep xmllint
xml_grep xmllint: EXPLAIN:=PortageII tmx2lfl.pl required xml_grep and xmllint. Make sure they are correcly installed.
xml_grep xmllint:
	@$(call check_installed,$@)
	@$(call check,$@ runs,\
		$@ --version 2>&1 | grep -q "$@.*version",\
		"$@ --version FAILED. ${EXPLAININSTALL}")

# Since the ram is power of 1024 but swap is power of 1000, we will make the
# conversion here.
.PHONY: ram_vs_swap
ram_vs_swap:
	-@$(call check,Swap space,\
		SWAP=`cat /proc/meminfo | grep SwapTotal | egrep -o '[0-9]+'`; \
		RAM=`cat /proc/meminfo | grep MemTotal | egrep -o '[0-9]+'`; \
		[[ $$SWAP -ge $$((2 * $$RAM * 1000 / 1024 * 1000 / 1024)) ]], \
		TOO SMALL. We strongly suggest that you have twice as much swap as RAM.)


########################################
# What needs to work after a successful installation of Portage.
testsuite: post_installation

VALIDATE_PGMS := \
    canoe tune.py utokenize.pl ce.pl tmx2lfl.pl filter-nc1.py casemark.py \
    arpalm.encode run-parallel.sh
.PHONY: post_installation
post_installation: ${VALIDATE_PGMS}
	@echo $$'\nPortageII itself is correctly installed.\n'

canoe: EXPLAIN:=Make sure Portage is correctly installed.
ce.pl: EXPLAIN:=Check your PERL5LIB or XML::Twig may not be correctly installed.
tune.py: EXPLAIN:=Check your Python version. Make sure Portage is correctly installed.
tmx2lfl.pl: EXPLAIN:=Check your PERL5LIB, or you're missing XML::Twig, or your XML::Twig version is less than 3.32, and make sure PortageTextProcessing is installed.
utokenize.pl: EXPLAIN:=Check your PERL5LIB and make sure PortageTextProcessing is installed.
filter-nc1.py: EXPLAIN:=Check your Python version and PYTHONPATH.
casemark.py: EXPLAIN:=Check your Python version and PYTHONPATH. Make sure Python is installed correctly (with dynamic library support).
arpalm.encode: EXPLAIN:=Check that all boost libraries are installed.
run-parallel.sh: EXPLAIN:=Make sure PortageClusterUtils is installed.

.PHONY: ${VALIDATE_PGMS}
# We simply make sure we can invoke the help message to trigger the dependencies.
${VALIDATE_PGMS}:
	@$(call check,$@ installed,which $@ &> /dev/null,\
		"NOT FOUND. Cannot locate PortageII program $@ in your PATH. Refer to \$$PORTAGE/INSTALL.")
	@$(call check,$@ runs,$@ -h >& /dev/null,"$@ -h FAILED TO RUN: ${EXPLAININSTALL}")
	@$(call check,$@ help,$@ -h 2>&1 | head -5 | diff - ref/$@.ref,\
		"$@ -h did not produce the expected output: Is it the right version? ${EXPLAININSTALL}")

post_installation: check_casemark

.PHONY: check_casemark
check_casemark: EXPLAIN:=Check your LD_LIBRARY_PATH: it should include the location of libportage_truecasing.so. \
                         Check your PYTHONPATH: it should include the location of portage_utils.pyc. \
                         Make sure Python is installed correctly (with dynamic library support).
check_casemark:
	@$(call check,casemark.py prerequisites, \
		casemark.py -lm /dev/null /dev/null 2>&1 | diff - ref/casemark.py.error-ref, \
		"ERROR. ${EXPLAININSTALL}")

post_installation: check_structpred

.PHONY: check_structpred
check_structpred: EXPLAIN:=Make sure Java is correctly installed.
check_structpred:
	@$(call check,structpred.jar installed,which structpred.jar &> /dev/null,\
		"Can't locate structpred.jar in your PATH. Refer to \$$PORTAGE/INSTALL.")
	@$(call check,structpred runs, \
		java -Xmx1000m -jar `which structpred.jar` JavaVersion 2>&1 | grep -q "StructPred version", \
		"FAILED. ${EXPLAININSTALL}")
