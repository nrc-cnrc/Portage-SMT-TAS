#!/usr/bin/make -f
# vim:noet:ts=3

.SUFFIXES:

LIST_TARGETS ?= bleucompare cow main rat train_compare

all: $(addprefix do.,${LIST_TARGETS})

do.%:
	${MAKE} -C $*
