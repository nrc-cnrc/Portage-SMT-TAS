#!/usr/bin/make -f
# vim:noet:ts=3

.PHONY: all
all: test


.PHONY: clean
clean:
	${RM} -rf lm.tplm lm.tplm.tmp.*


.PHONY: test
test:
	arpalm2tplm.sh -v lm
