#!/usr/bin/make -f
# vim:noet:ts=3

LIST_TARGETS ?= bleucompare main rat cow

.SUFFIXES:

all: $(addprefix do.,${LIST_TARGETS})

do.%:
	${MAKE} -C $*
