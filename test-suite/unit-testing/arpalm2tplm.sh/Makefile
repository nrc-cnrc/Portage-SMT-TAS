#!/usr/bin/make -f
# vim:noet:ts=3

.PHONY: all
all: test fixed
# all: bug

# lm has a ^K special character inside a token - breaks arpalm2tplm.sh
bug: bug-special-char
bug-special-char: diff.lm

# lm2 has not special characters but is missing some BO weights - breaks
# arpalm2tplm.sh because it requires stricly correct ARPA LM input.
bug: bug-missing-bo
bug-missing-bo: diff.lm2

# diff.lm2-bo now works: arpalm2tplm.sh works correctly when the input LM
# strictly respects the ARPA LM file specs, which requires that every context
# and suffix of context have an explicit back-off weight, even when 0.
# binlm2arplm can now be used to insert the missing 0's in a existing ARPA LM
# file.
fixed: fixed-bo-inserted
fixed-bo-inserted: diff.lm2-bo

.SECONDARY:


TEMP_FILES=log.* out.* lm-bo lm2-bo
TEMP_DIRS=*.tplm*
include ../Makefile.incl


.PHONY: test
test:
	arpalm2tplm.sh -v lm >& log.$@

out.%: %
	lm_eval -v $* input > $@ 2> log.$@

diff.%: out.% out.%.tplm
	diff $+

lm%.tplm: lm%
	arpalm2tplm.sh -v $< >& log.$@

lm%-bo: lm%
	binlm2arpalm $< $@ >& log.$@
