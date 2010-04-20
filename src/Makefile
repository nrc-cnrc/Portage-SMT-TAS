# Portage top-level Makefile
#
# @author George Foster, Eric Joanis, Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

.PHONY: all clean install export docs test lib
all clean install export docs test lib:
	$(MAKE) OT=$@ subdirs

# Targets new and new_install need to be custom defined here, because
# "make OT=new subdirs" doesn't have exactly the same semantics as doing
# "make new" "make all".  (This difference only matters when -j is used.)

new:
	$(MAKE) OT=clean subdirs
	$(MAKE) OT=all subdirs

new_install:
	$(MAKE) OT=clean subdirs
	$(MAKE) OT=install subdirs

# These directories will be compiled by default
# IMPORTANT NOTE: when you add a directory here, you must also add it to the
# DIRECTORY DEPENDENCIES section below.
SUBDIRS = adaptation api canoe confidence distortion eval \
          lm logging preprocessing rescoring \
          tm tpt truecasing textutils

# Additional directories listed here are only compiled if explicitly
# specified.  Use this for modules in development that are not ready to be
# generally used yet.
# Directories listed here must also be covered in DIRECTORY DEPENDENCIES.
ALL_SUBDIRS = $(SUBDIRS)

.PHONY: subdirs utils $(ALL_SUBDIRS) validate_subdir_dependencies

subdirs: validate_subdir_dependencies utils $(SUBDIRS)

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

.PHONY: FORCE
FORCE::

# Automatically calculate the dependencies for each module, by reading its
# Makefile and consulting its MODULE_DEPENDS variable
calc_deps = $(shell perl -pe 's/\\\n$$/ /' < $(1)/Makefile | grep MODULE_DEPENDS | sed 's/ *MODULE_DEPENDS *= *//')

# DIRECTORY DEPENDENCIES
# Each directory in $(SUBDIRS) and $(ALL_SUBDIRS) must be listed here, even if
# it does not currently have any dependencies.
adaptation:     $(call calc_deps,adaptation)
api:            $(call calc_deps,api)
canoe:          $(call calc_deps,canoe)
confidence:     $(call calc_deps,confidence)
distortion:     $(call calc_deps,distortion)
eval:           $(call calc_deps,eval)
lm:             $(call calc_deps,lm)
logging:        $(call calc_deps,logging)
preprocessing:  $(call calc_deps,preprocessing)
rescoring:      $(call calc_deps,rescoring)
tm:             $(call calc_deps,tm)
tpt:            $(call calc_deps,tpt)
truecasing:     $(call calc_deps,truecasing)
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
	| ./utils/pgm_usage_2_html.pl -main "Portage v1.4: Usage information by program" \
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

