#!/usr/bin/make -f
# vim:noet:ts=3:nowrap
# $Id$

# @file Makefile.bin
# @brief Assembles all the minimal set of software that is required for PortageLive.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologiesm
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

.DEFAULT_GOAL := all

# Allow for user specific parameters.
-include Makefile.params


# Where we want to put the minimum set of software required for PortageLive to
# package it into a rpm.
DEST_BASE_DIR ?= rpm.build.root/opt/Portage


################################################################################
# BINARIES
CE ?= ce_canoe2ffvals.pl ce_gen_features.pl ce.pl ce_tmx.pl ce_train.pl ce_translate.pl ce_ttx2ospl.pl plog.pl
CE_EXTERNAL ?= svm-predict
CLUSTER ?= parallelize.pl psub r-parallel-d.pl r-parallel-worker.pl run-parallel.sh which-test.sh
DECODE ?= canoe canoe-escapes.pl canoe-parallel.sh
TRANSLATE ?= postdecode_plugin postprocess_plugin predecode_plugin preprocess_plugin translate.pl ridbom.sh crlf2lf.sh utf8_filter
OTHER_BIN ?= configtool portage_info
RESCORE ?= feature_function_tool gen-features-parallel.pl gen_feature_values nbest2rescore.pl rat.sh rescore_translate
TOKENIZATION ?= truecase.pl udetokenize.pl utf8_casemap utokenize.pl
EXTRAS ?= bloater

# Must contains pgm from Portage only.
PORTAGE_BINS ?= ${CE} ${DECODE} ${OTHER_BIN} ${RESCORE} ${TOKENIZATION} ${TRANSLATE}
CLUSTER_BINS ?= ${CLUSTER}
BINARIES ?= ${PORTAGE_BINS} ${CLUSTER_BINS} ${CE_EXTERNAL}


# Make sure all Portage's binaries are from the same source build.
SRC_PORTAGE_BIN_DIR := $(sort $(dir $(shell which ${PORTAGE_BINS} 2> /dev/null)))
ifeq ("$(words ${SRC_PORTAGE_BIN_DIR})","0")
$(error Problem finding Portage software, maybe it's not installed)
else ifneq ("$(words ${SRC_PORTAGE_BIN_DIR})","1")
$(error Problem finding Portage software, maybe it's not installed, or the PORTAGE_BINS variable lists non-Portage programs)
endif

# Where are we supposed to copy the binaires?
DEST_BIN_DIR ?= ${DEST_BASE_DIR}/bin

########################################
# How to handle binaries by defining binaries dependencies.
# We should be able to find the binaries in the current PATH.
vpath % ${PATH}

INSTALL_BIN_LIST := $(addprefix ${DEST_BIN_DIR}/, ${BINARIES})

.PHONY: install_bin
all: install_bin
install_bin: ${INSTALL_BIN_LIST}
	chmod 0755 $+

# Binaries' dependencies.
${INSTALL_BIN_LIST}: ${DEST_BIN_DIR}/%: %

ALL_FILES += ${INSTALL_BIN_LIST}

# plog.pl requires an accounting directory
${DEST_BIN_DIR}/plog.pl:  ${DEST_BASE_DIR}/logs/accounting
${DEST_BASE_DIR}/logs/accounting:
	mkdir -p $@
	chmod 0755 $(dir $@)
	chmod 0777 $@



################################################################################
# OTHER FILES
OTHER ?= 
DEST_OTHER_DIR ?= ${DEST_BASE_DIR}

INSTALL_OTHER_LIST := $(addprefix ${DEST_OTHER_DIR}/, ${OTHER})

.PHONY: install_other
install_other: ${DEST_OTHER_DIR}/SETUP.bash
install_other: ${INSTALL_OTHER_LIST}
all: install_other

# Others' dependencies
${INSTALL_OTHER_LIST}: ${DEST_OTHER_DIR}/%: %

ALL_FILES += ${INSTALL_OTHER_LIST}

# Special for a custom setup file.
${DEST_OTHER_DIR}/SETUP.bash:
	echo "export LD_LIBRARY_PATH=\$${PORTAGE}/lib:\$$LD_LIBRARY_PATH" > $@
	echo "export PATH=\$${PORTAGE}/bin:\$$PATH" >> $@
	echo "export PERL5LIB=\$${PORTAGE}/lib:\$$PERL5LIB" >> $@
	chmod 0755 $@



################################################################################
# LIBRARIES
#PORTAGE_LIBS := portage_truecaselibconstantes.pm portage_truecaselib.pm portage_utils.pm ULexiTools.pm
PORTAGE_LIBS ?= portage_truecaselibconstantes.pm portage_truecaselib.pm portage_utils.pm ULexiTools.pm
LIBRARIES += ${PORTAGE_LIBS}

# Define DO_EXTERNAL_LIBS if you want to bundle all external libs need by
# Portage's software.
ifdef DO_EXTERNAL_LIBS
# Here, we fetch the libraries related to Portage's pgm only.
EXTERNAL_LIBS := $(sort $(notdir $(shell ldd `which ${PORTAGE_BINS} ${CLUSTER_BINS} 2> /dev/null` | egrep -i Portage | perl -nle 'print $$1 if /=> ([^ ]+)/;' | sort | uniq)))
LIBRARIES += ${EXTERNAL_LIBS}
EXTERNAL_LIBS_DIR := $(sort $(dir $(shell ldd `which ${PORTAGE_BINS} ${CLUSTER_BINS} 2> /dev/null` | egrep -i Portage | perl -nle 'print $$1 if /=> ([^ ]+)/;' | sort | uniq)))
vpath % ${EXTERNAL_LIBS_DIR}
endif

# Portage's libraries are just beside the binaries' directory.
# Note: this can represent more than one directory.
SRC_PORTAGE_LIB_DIR ?= ${SRC_PORTAGE_BIN_DIR:bin/=lib}

# Where are we supposed to copy the libraries.
DEST_LIB_DIR ?= ${DEST_BASE_DIR}/lib

########################################
# How to handle libraries.
vpath % ${SRC_PORTAGE_LIB_DIR}

# Where and what are the libraries needed.
INSTALL_LIB_LIST := $(addprefix ${DEST_LIB_DIR}/, ${LIBRARIES})
.PHONY: install_lib
all: install_lib
install_lib: ${INSTALL_LIB_LIST}
install_lib: ${DEST_LIB_DIR}/CE
	chmod 0755 $(filter-out %.pm, $+)
	chmod 0644 $(filter %.pm, $+)

# Libraries' dependencies.
${INSTALL_LIB_LIST}: ${DEST_LIB_DIR}/%: %

ALL_FILES += ${INSTALL_LIB_LIST}

# Special rule to install Confidence Estimation's plugins.
${DEST_LIB_DIR}/CE: ${DEST_LIB_DIR}/%: ${SRC_PORTAGE_LIB_DIR}/%
	cp -R $< $@
	chmod 0755 $@
	chmod 0644 $@/*.pm



################################################################################
# Now that we defined all that we need, we can define the all target.
SETUP_DEST_DIR := $(shell mkdir -p ${DEST_BIN_DIR} ${DEST_LIB_DIR})

.PHONY: all
all:
	chmod 0755 ${DEST_BASE_DIR} $(dir ${DEST_BASE_DIR}) ${DEST_BIN_DIR} ${DEST_LIB_DIR}

# Basically, it's all about copying.
${ALL_FILES}: %:
	cp $< $@


################################################################################
# The notices file has to get assembled from the head, tail, and the output of
# portage_info -notice
.PHONY: notice
all: notice
notice: notices_head.html notices_tail.html
	mkdir -p rpm.build.root/var/www/html
	find rpm.build.root/var -type d | xargs chmod 0755
	cat notices_head.html > rpm.build.root/var/www/html/portage_notices.html
	portage_info -notice >> rpm.build.root/var/www/html/portage_notices.html
	cat notices_tail.html >> rpm.build.root/var/www/html/portage_notices.html
	chmod 0644 rpm.build.root/var/www/html/portage_notices.html
	portage_info -notice >> rpm.build.root/opt/Portage/NOTICE



################################################################################
# CLEAN UP.
.PHONY: clean
clean:
	${RM} -r ${DEST_BASE_DIR}



################################################################################
# HELPERS
.PHONY: debug
debug:
	@echo "all files:"
	@echo "${ALL_FILES}" | tr ' ' '\n'
	@echo "Needed libraries:"
	@echo "${EXTERNAL_LIBS}"
	@echo "Needed libraries dir:"
	@echo "${EXTERNAL_LIBS_DIR}"
	@echo "SRC_PORTAGE_LIB_DIR: ${SRC_PORTAGE_LIB_DIR}"
	@echo "PORTAGE_BINS: ${PORTAGE_BINS}"
	@echo "WHICH: $(shell which ${PORTAGE_BINS})"
	@echo ${PORTAGE}

