#!/usr/bin/make -f
# vim:noet:ts=3

.SUFFIXES:

LIST_TARGETS ?= bleucompare main rat cow

all: $(addprefix do.,${LIST_TARGETS})

do.%:
	${MAKE} -C $*

clean:
	${MAKE} clean -C rat
	${MAKE} clean -C cow
