#!/usr/bin/make -f
# vim:noet:ts=3:nowrap

# @file Makefile.bin
# @brief Assembles all the minimal set of software that is required for PortageLive.
#
# Invoke as "make DO_EXTERNAL_LIBS=1" to include external libraries in the result.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

.DEFAULT_GOAL := all

# Allow for user specific parameters.
-include Makefile.params

TARBALL ?= portagelive.bin.tgz

# Where we want to put the minimum set of software required for PortageLive to
# package it into a rpm.
ROOT_DIR ?= rpm.build.root
DEST_BASE_DIR ?= ${ROOT_DIR}/opt/PortageII
HTML_DIR ?= ${ROOT_DIR}/var/www/html


################################################################################
# BINARIES AND EXECUTABLE SCRIPTS
#If NRC's rewrite of the LDC/UPENN Chinese segmenter is installed, include it.
CHINESE ?= chinese_rule_create.pl chinese_rule_markup.pl ictclas_postprocessing.pl ictclas_preprocessing.pl ictclas_run.sh \
           $(notdir $(shell which chinese_segmenter.pl 2> /dev/null))
CE ?= ce_canoe2ffvals.pl ce_gen_features.pl ce.pl ce_tmx.pl ce_train.pl ce_translate.pl ce_ttx2ospl.pl plog.pl
CE_EXTERNAL ?= svm-predict
CLUSTER ?= parallelize.pl psub r-parallel-d.pl r-parallel-worker.pl run-parallel.sh \
           which-test.sh time-mem sum.pl process-memory-usage.pl
DECODE_BIN ?= canoe
DECODE_SCRIPTS ?= canoe-escapes.pl canoe-parallel.sh sh_utils.sh
# NOTE: DO NOT add detokenize_plugin script to the BINSCRIPTS variable since
# it exists only to illustrate what a detokenize_plugin could look like.
TRANSLATE ?= postdecode_plugin postprocess_plugin predecode_plugin preprocess_plugin sentsplit_plugin tokenize_plugin \
             sentsplit-with-tags-combine.pl sentsplit-with-tags-split.pl tok-with-tags-combine.pl tok-with-tags-split.pl \
             translate.pl ridbom.sh crlf2lf.sh fix-en-fr-numbers.pl add-fr-nbsp.pl \
             mark-numbers-en2fr.pl mark-numbers-fr2en.pl fixed_term2tm.pl
OTHER_BIN ?= utf8_filter utf8_casemap configtool portage_info normc1 markup_canoe_output
RESCORE_BIN ?= feature_function_tool gen_feature_values rescore_translate
RESCORE_SCRIPTS ?= gen-features-parallel.pl nbest2rescore.pl rat.sh
TOKENIZATION ?= udetokenize.pl utokenize.pl parse_tokan.pl
TRUECASING ?= truecase.pl casemark.py boscap.py boscap-nosrc.py
#Define EXTRAS in Makefile.params to include extra PortageII programs
EXTRAS ?=

# Must contain programs from PortageII only.
PORTAGE_SCRIPTS ?= ${CHINESE} ${CE} ${DECODE_SCRIPTS} ${TRANSLATE} ${RESCORE_SCRIPTS} ${TOKENIZATION} ${TRUECASING}
PORTAGE_BINS ?= ${DECODE_BIN} ${OTHER_BIN} ${RESCORE_BIN} ${EXTRAS}
CLUSTER_BINS ?= ${CLUSTER}
BINARIES ?= ${PORTAGE_SCRIPTS} ${PORTAGE_BINS} ${CLUSTER_BINS} ${CE_EXTERNAL}


# Make sure all PortageII's binaries are from the same source build.
SRC_PORTAGE_BIN_DIR := $(sort $(dir $(shell which ${PORTAGE_BINS} 2> /dev/null)))
SRC_PORTAGE_BIN_DIR := ${SRC_PORTAGE_BIN_DIR:/=}
ifeq ("$(words ${SRC_PORTAGE_BIN_DIR})","0")
   $(error Problem finding PortageII binary software, maybe it's not installed)
else ifneq ("$(words ${SRC_PORTAGE_BIN_DIR})","1")
   $(error Finding PortageII binary software in multiple directories, make sure you're pointing to a single consistent installation)
endif

# Make sure all PortageII's scripts are from the same source build.
SRC_PORTAGE_SCRIPT_DIR := $(sort $(dir $(shell which ${PORTAGE_SCRIPTS} 2> /dev/null)))
SRC_PORTAGE_SCRIPT_DIR := ${SRC_PORTAGE_SCRIPT_DIR:/=}
ifeq ("$(words ${SRC_PORTAGE_SCRIPT_DIR})","0")
   $(error Problem finding PortageII scripts, maybe it's not installed)
else ifneq ("$(words ${SRC_PORTAGE_SCRIPT_DIR})","1")
   $(error Finding PortageII scripts in multiple directories, make sure you're pointing to a single consistent installation)
endif

ifneq (${SRC_PORTAGE_SCRIPT_DIR},${SRC_PORTAGE_BIN_DIR})
   ifneq (${SRC_PORTAGE_SCRIPT_DIR}/,$(dir ${SRC_PORTAGE_BIN_DIR}))
      $(error PortageII scripts and binary programs should be either in the same folder or in a bin folder and an architecture-specific subfolder)
   endif
   ARCH_TOKEN=$(notdir ${SRC_PORTAGE_BIN_DIR})
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

########################################
# Handle the INSTALL_SUMMARY file, if present.
ifneq ("$(wildcard ${SRC_PORTAGE_SCRIPT_DIR}/INSTALL_SUMMARY)","")
INSTALL_SUMMARY_LIST := $(addprefix ${DEST_BIN_DIR}/, INSTALL_SUMMARY)

.PHONY: install_summary
all: install_summary
install_summary: ${INSTALL_SUMMARY_LIST}
	chmod 0644 $+

# INSTALL_SUMMARY dependencies
${INSTALL_SUMMARY_LIST}: ${DEST_BIN_DIR}/%: ${SRC_PORTAGE_SCRIPT_DIR}/%

ALL_FILES += ${INSTALL_SUMMARY_LIST}
endif

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
	echo "export PYTHONPATH=\$${PORTAGE}/lib:\$$PYTHONPATH" >> $@
	chmod 0755 $@



################################################################################
# LIBRARIES
PORTAGE_LIBS ?= portage_utils.pm portage_utils.pyc ULexiTools.pm libportage_truecasing.so
#If NRC's rewrite of the LDC/UPENN Chinese segmenter is included, include manseg.fre.
PORTAGE_LIBS += $(if $(filter chinese_segmenter.pl, ${CHINESE}), manseg.fre)

# Define DO_EXTERNAL_LIBS if you want to bundle all external libs need by
# PortageII's software.
ifdef DO_EXTERNAL_LIBS
# Here, we fetch the libraries related to PortageII's pgm only.
EXTERNAL_LIBS := $(sort $(notdir $(shell ldd `which ${PORTAGE_BINS} ${CLUSTER_BINS} 2> /dev/null` | grep -v ' /lib' | perl -nle 'print $$1 if /=> ([^ ]+)/;' | sort | uniq)))
EXTERNAL_LIBS_DIR := $(sort $(dir $(shell ldd `which ${PORTAGE_BINS} ${CLUSTER_BINS} 2> /dev/null` | grep -v ' /lib' | perl -nle 'print $$1 if /=> ([^ ]+)/;' | sort | uniq)))
vpath % ${EXTERNAL_LIBS_DIR}
endif

# PortageII's libraries are just beside the binaries' directory.
# Note: this can represent more than one directory.
SRC_PORTAGE_LIB_DIR ?= ${SRC_PORTAGE_SCRIPT_DIR:bin=lib}

# Where are we supposed to copy the libraries.
DEST_LIB_DIR ?= ${DEST_BASE_DIR}/lib

########################################
# How to handle libraries.
ifdef ARCH_TOKEN
   vpath % ${SRC_PORTAGE_LIB_DIR} ${SRC_PORTAGE_LIB_DIR}/${ARCH_TOKEN}
else
   vpath % ${SRC_PORTAGE_LIB_DIR}
endif

# Where and what are the libraries needed.
PORTAGE_LIB_LIST := $(addprefix ${DEST_LIB_DIR}/, ${PORTAGE_LIBS})
EXTERNAL_LIB_LIST := $(addprefix ${DEST_LIB_DIR}/, ${EXTERNAL_LIBS})
INSTALL_LIB_LIST := ${PORTAGE_LIB_LIST} ${EXTERNAL_LIB_LIST}
.PHONY: install_lib
all: install_lib
install_lib: ${INSTALL_LIB_LIST}
install_lib: ${DEST_LIB_DIR}/CE
ifdef DO_EXTERNAL_LIBS
	$(if $(wildcard ${DEST_LIB_DIR}/libstdc++.so.*), ln -fs $(firstword $(notdir $(wildcard ${DEST_LIB_DIR}/libstdc++.so.*))) ${DEST_LIB_DIR}/libstdc++.so)
endif
	chmod 0755 $(filter-out %.pm, $+)
	chmod 0644 $(filter %.pm, $+)

# Libraries' dependencies.
${INSTALL_LIB_LIST}: ${DEST_LIB_DIR}/%: %

ALL_FILES += ${PORTAGE_LIB_LIST}

# Special rule to install external libraries
ifeq ($(shell which /usr/sbin/prelink),"/usr/sbin/prelink")
# Note: must first copy the file because prelink wants to retain the original owner
${EXTERNAL_LIB_LIST}: ${DEST_LIB_DIR}/%:
	cp $< $@.tmp
	/usr/sbin/prelink -uo $@ $@.tmp
	rm $@.tmp
else
${EXTERNAL_LIB_LIST}: ${DEST_LIB_DIR}/%:
	cp $< $@
endif

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
	mkdir -p ${HTML_DIR}
	find ${ROOT_DIR}/var -type d | xargs chmod 0755
	cat notices_head.html > ${HTML_DIR}/portage_notices.html
	portage_info -notice >> ${HTML_DIR}/portage_notices.html
	cat notices_tail.html >> ${HTML_DIR}/portage_notices.html
	chmod 0644 ${HTML_DIR}/portage_notices.html
	portage_info -notice >> ${DEST_BASE_DIR}/NOTICE



################################################################################
# CLEAN UP.
.PHONY: clean
clean:
	${RM} -r ${ROOT_DIR}
	${RM} ${TARBALL}



################################################################################
# tarball portagelive's binaries.
.PHONY:  tarball
tarball: ${TARBALL}

${TARBALL}:  all
	cd ${ROOT_DIR} && tar zchvf ../$@ * && cd ..




################################################################################
# HELPERS
.PHONY: debug
debug:
	@echo "all files:"
	@echo "${ALL_FILES} $(wildcard ${DEST_LIB_DIR}/CE/*) ${EXTERNAL_LIB_LIST}" | tr ' ' '\n'
	@echo -e "\nNeeded libraries:"
	@echo "${EXTERNAL_LIBS}"
	@echo "Needed libraries dir:"
	@echo "${EXTERNAL_LIBS_DIR}"
	@echo -e "\nSRC_PORTAGE_LIB_DIR: ${SRC_PORTAGE_LIB_DIR}"
	@echo "PORTAGE_BINS: ${PORTAGE_BINS}"
	@echo "WHICH: $(shell which ${PORTAGE_BINS})"
	@echo "PORTAGE: ${PORTAGE}"

