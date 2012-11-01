#!/usr/bin/make -f
# vim:noet:ts=3

.PHONY: all
all: test

TEMP_DIRS=lm.tplm lm.tplm.tmp.*
include ../Makefile.incl

.PHONY: test
test:
	arpalm2tplm.sh -v lm
