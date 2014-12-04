# PortageII top-level Makefile
#
# @author George Foster, Eric Joanis, Samuel Larkin, Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

.PHONY: all clean install export docs test lib new new_install

all clean docs export lib test: %: parallel/%
	@true

install: parallel/install
	$(MAKE) install_summary

new: clean
	$(MAKE) all

new_install: clean
	$(MAKE) install

# These directories will be compiled by default
# IMPORTANT NOTE: when you add a directory here, you must also add it to the
# DIRECTORY DEPENDENCIES section below.
SUBDIRS = adaptation \
	  canoe \
	  confidence \
	  distortion \
	  eval \
	  lm \
	  logging \
	  preprocessing \
	  rescoring \
	  tm \
	  tp_models \
	  tpt \
	  truecasing \
	  textutils \
	  word_align

# Additional directories listed here are only compiled if explicitly
# specified.  Use this for modules in development that are not ready to be
# generally used yet.
# Directories listed here must also be covered in DIRECTORY DEPENDENCIES.
ALL_SUBDIRS = $(SUBDIRS)

.PHONY: subdirs utils $(ALL_SUBDIRS) validate_subdir_dependencies

subdirs: validate_subdir_dependencies utils $(SUBDIRS)
ifeq (${OT},install)
	$(MAKE) install_summary
# Here, we want to list PortageII's pgms that are obsolete but are still present
# in the installation directory of the user unless the user installed PortageII
# in his home/bin in which case we prefer to be silent.
ifneq (${BIN_DIR},${HOME}/bin)
	@LIST_OBSOLETE=`find ${BIN_DIR} $(dir ${BIN_DIR})lib -ctime +1`; \
	[[ `echo $$LIST_OBSOLETE | \wc -w` != 0 ]] \
	&& { echo $$LIST_OBSOLETE | tr " " "\n"; echo "Warning: These files are most likely obsolete and we recommend deleting them."; }; \
	unset LIST_OBSOLETE
endif
endif


# The install_summary target is invoked by install targets to update the 
# INSTALL_SUMMARY file in the bin directory to reflect the Git status of 
# the working tree corresponding to the installed items.

BIN_DIR = ../bin
INSTALL_PATH = $(if ${MODULE_NAME}, $(abspath ${MODULE_NAME}), ${CURDIR})

.PHONY: install_summary
ifneq ("$(shell git rev-parse --is-inside-work-tree 2> /dev/null)","true")
install_summary:
	@:
else

ifneq ($(filter install_summary_helper,${MAKECMDGOALS}),)
   # These calculations can be expensive, in particular setting GIT_RBRANCH, so
   # shield them so they're only evaluated when needed.
   GIT_RBRANCH := $(strip $(shell for rev in $(shell git rev-list HEAD | head -1000); do if git branch -r --contains $$rev | grep -m 1 -v -- '->'; then break; fi done))
   GIT_REMOTE := $(notdir $(patsubst %/,%,$(dir $(GIT_RBRANCH))))
   GIT_RBRANCH_BASE := $(shell git merge-base HEAD ${GIT_RBRANCH})
endif

install_summary:
	$(if ${MODULE_NAME},echo "" >>,cat /dev/null >) ${BIN_DIR}/INSTALL_SUMMARY
	-${MAKE} --no-print-directory install_summary_helper >> ${BIN_DIR}/INSTALL_SUMMARY 2>&1

install_summary_helper: SHELL=/bin/bash
install_summary_helper:
	@date
	@echo "$(if ${MODULE_NAME},Module installed,Sandbox): ${INSTALL_PATH}"
	@echo "Git working directory: `git rev-parse --show-toplevel`"
	@echo "Git branch:            `git rev-parse --abbrev-ref HEAD`"
	@echo "Closest remote branch: ${GIT_RBRANCH}"
	@echo "Git branch base:       ${GIT_RBRANCH_BASE}"
	@echo ""
	git remote show ${GIT_REMOTE}
	@echo ""
	git branch -r -v --no-abbrev
	@echo ""
	@echo "HEAD:"
	git show HEAD --no-abbrev --format=fuller --summary --decorate
	git status -u ${INSTALL_PATH}
	@echo ""
	-git diff -p --stat ${GIT_RBRANCH_BASE} -- ${INSTALL_PATH}
endif


vpath Doxyfile build/
doxy: Doxyfile
	( cat $<; echo "HTML_FOOTER=build/footer.txt" ) | doxygen -

utils: validate_subdir_dependencies
	$(MAKE) -C $@ $(OT)

$(ALL_SUBDIRS): utils
	$(MAKE) -C $@ $(OT)

# Target used to make just the strict minimum to build the programs in one
# directory: its dependent libraries, its own library, and its executables.
%/progs: DEPENDENT_LIBS=utils $(call calc_deps,$*) $*
%/progs:
	$(MAKE) $(foreach D,$(DEPENDENT_LIBS),$D/lib)
	$(MAKE) -C $* $(OT)
ifeq (${OT},install)
	$(MAKE) MODULE_NAME=$* install_summary
endif

# Target used to make just the library (.a file) in a subdirectory
%/lib:
	$(MAKE) -C $* lib

# This target allows the syntax <dir>/<program>, e.g., canoe/canoe or
# tm/train_ibm, to do the strict minimum amount of work required to compile
# that one program: the dependent libraries, the library in <dir>, and
# <program> itself in <dir>.
DIR_CMD=$(foreach D,$(ALL_SUBDIRS) utils,$D/%)
$(DIR_CMD): FORCE
	$(MAKE) OT=$* $(word 1,$(subst /, ,$@))/progs

# Generate rules equivalent the pattern %_CMD_%, e.g., dir_CMD_target, which
# will run target in dir.
$(foreach D,$(ALL_SUBDIRS) utils $(IIT_SHARED_UTILS), \
   $(eval \
      $D_CMD_%: FORCE ; $$(MAKE) -C $D $$* \
   ) \
)

parallel/%: parallel/lib
	${MAKE} $(foreach D,utils $(IIT_SHARED_UTILS) $(SUBDIRS),$D_CMD_$*)

parallel/lib parallel/clean parallel/docs: parallel/%: $(foreach D,utils $(SUBDIRS),$D_CMD_%)
	@true

.PHONY: FORCE
FORCE::

# Automatically calculate the dependencies for each module, by reading its
# Makefile and consulting its MODULE_DEPENDS variable
calc_deps = $(shell perl -pe 's/\\\n$$/ /' < $(1)/Makefile | grep MODULE_DEPENDS | sed 's/ *MODULE_DEPENDS *= *//')

# DIRECTORY DEPENDENCIES
# Each directory in $(SUBDIRS) and $(ALL_SUBDIRS) must be listed here, even if
# it does not currently have any dependencies.
adaptation:     $(call calc_deps,adaptation)
canoe:          $(call calc_deps,canoe)
confidence:     $(call calc_deps,confidence)
distortion:     $(call calc_deps,distortion)
eval:           $(call calc_deps,eval)
lm:             $(call calc_deps,lm)
logging:        $(call calc_deps,logging)
preprocessing:  $(call calc_deps,preprocessing)
rescoring:      $(call calc_deps,rescoring)
textutils:      $(call calc_deps,textutils)
tm:             $(call calc_deps,tm)
tp_models:      $(call calc_deps,tp_models)
tpt:            $(call calc_deps,tpt)
truecasing:     $(call calc_deps,truecasing)
word_align:     $(call calc_deps,word_align)
# EJJ design note: can't figure out how to make the list above automatically
# apply to everything in $(ALL_SUBDIRS), unfortunately, so we'll still have to
# manually add each directory in both places, but at least the actual list of
# dependencies is now calculated.


# This next section is to generate web pages from the programs help message.

# Indicates where the web pages should reside.
USAGE_DIR ?= ../doc/usage
#export USAGE_DIR := $(eval $(abspath ${USAGE_DIR}))

# Main alias to create the web pages.
.PHONY: usage
usage: list.html
index.html: make_module_usage
	find ${USAGE_DIR} -mindepth 2 -maxdepth 2 -name index.html \
	| xargs -i dirname {} \
	| sed 's#.*/##' \
	| tr " " "\n" \
	| sort \
	| ./utils/pgm_usage_2_html.pl -main "PortageII 2.2: Usage information by program" \
	> ${USAGE_DIR}/$@
	cp build/*.gif build/*.jpg ${USAGE_DIR}

list.html: index.html
	find ${USAGE_DIR} -mindepth 2 -maxdepth 2 -name index.html \
	| xargs cat \
	| egrep '<TR ID="program brief description">' \
	| ./utils/pgm_usage_2_html.pl -index "List of all programs" \
	> ${USAGE_DIR}/$@

# Asks all modules to generate its own index and web pages.
.PHONY: make_module_usage
make_module_usage:
	${MAKE} USAGE_DIR=../${USAGE_DIR} OT=usage ${ALL_SUBDIRS}

.PHONY: clean_usage
clean_usage:
	${MAKE} USAGE_DIR=../${USAGE_DIR} OT=$@ ${ALL_SUBDIRS}
	${RM} ${USAGE_DIR}/index.html

.PHONY: new_usage
new_usage:
	${MAKE} clean_usage
	${MAKE} usage

