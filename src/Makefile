# Portage top-level Makefile
#
# George Foster, Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

all clean install export docs:
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
SUBDIRS = logging preprocessing eval lm tm adaptation canoe rescoring api

# Additional directories listed here are only compiled if explicitly
# specified.  Use this for modules in development that are not ready to be
# generally used yet.
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

%_progs:
	$(MAKE) OT=lib $*
	$(MAKE) -C $* $(OT)

# If you create a new subdirectory, add its dependencies here (except for
# utils, which everything is assumed to depend on)

canoe: logging tm lm

api: canoe tm lm logging

rescoring: canoe tm lm eval logging

tm: logging

eval: logging

validate_subdir_dependencies:
	@if [ "`grep 'MODULE_DEPENDS.*[^ =]' */Makefile | perl -ple 's/\/Makefile//; s/MODULE_DEPENDS\s*=/ /; s/\s+/ /' | wc`" \
	     != \
	     "`grep 'MODULE_DEPENDS.*[^ =]' */Makefile | perl -ple 's/\/Makefile//; s/MODULE_DEPENDS\s*=/ /; s/\s+/ /' | grep -x -f - Makefile  | wc`" \
	   ]; then \
	   echo There is an error in the module dependency block in src/Makefile. ; \
	   echo Please update it as follows '(you can cut and paste the following lines)': ; \
	   grep 'MODULE_DEPENDS.*[^ =]' */Makefile | \
	      perl -ple 's/\/Makefile//; s/MODULE_DEPENDS\s*=/ /; s/\s+/ /' | \
	      diff - Makefile | grep '^<' | sed 's/^< //' ; \
	   exit 1 ; \
	fi


