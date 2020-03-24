#!/usr/bin/make -f
# vim:ts=3:noet

# Makefile - Test (u)tokenize.pl and (u)detokenize.pl for various special cases.
#
# PROGRAMMER: Samuel Larkin / Eric Joanis / Darlene Stewart
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada


.SUFFIXES:
.PHONY: all french-hyphens dots ch-punc ch-punc-map newline
.SECONDARY:
export PORTAGE_INTERNAL_CALL=1
UTOKENIZE := utokenize.pl
UDETOKENIZE := udetokenize.pl
TOKENIZE := tokenize.pl
DETOKENIZE := detokenize.pl

all: test

.PHONY: test
test: display.accent \
     $(addprefix compare., accent.tok accent.utf8 accent.utf8.tok) \
     $(addprefix display., accent.tok accent.utf8 accent.utf8.tok) \
     french-hyphens dots ch-punc ch-punc-map en-prices smart-apos


french-hyphens: compare.hyphens.tok compare.hyphens.tok.utf8 \
                compare.hyphens.detok compare.hyphens.detok.utf8

dots: compare.periods.tok compare.periods.tok.utf8 \
      compare.periods.detok compare.periods.detok.utf8

ch-punc: $(addprefix compare.chinese-punc.detok., 000 001 010 011 100 101 110 111)

ch-punc-map: $(addprefix compare.ch-punc-utf8., cp1252 latin1 ascii)

.PHONY: en-prices
en-prices: compare.eng-prices.detok compare.eng-prices.detok.utf8

TEMP_FILES=accent.* hyphens.* periods.* chinese-punc.* eng-prices.* \
           es.* da.* xtags.* smart-apos.* paragraph.* \
           brackets.utf8.detok-?? ch-punc-utf8.* \
           with_tags.in with_tags.out empty.paragraph*
include ../Makefile.incl

accent.utf8: accent
	iconv -f iso-8859-1 -t UTF-8 < $< > $@

accent.tok: accent
	${TOKENIZE} -ss $< $@

accent.utf8.tok: accent.utf8
	${UTOKENIZE} -ss $< $@

display.%: %
	file $*
	hexdump -C $*

compare.%: %
	@echo -n "diff $* ref/$* "
	@(diff $* ref/$* &> /dev/null) && echo "OK" || (echo "FAILED" && false)


# hyphens specific.
hyphens.utf8: hyphens
	iconv -f cp1252 -t utf-8 < $< > $@

hyphens.tok: hyphens
	${TOKENIZE} -lang=fr -ss $< > $@

hyphens.detok: hyphens.tok
	${DETOKENIZE} -lang=fr $< > $@

hyphens.tok.utf8: hyphens.utf8
	${UTOKENIZE} -ss -lang=fr < $< > $@

hyphens.detok.utf8: hyphens.tok.utf8
	${UDETOKENIZE} -lang=fr $< > $@

# periods specific.
periods.utf8: periods
	iconv -f cp1252 -t utf-8 < $< > $@

periods.tok: periods
	${TOKENIZE} -lang=fr -ss -paraline $< > $@

periods.detok: periods.tok
	${DETOKENIZE} -lang=fr $< > $@

periods.tok.utf8: periods.utf8
	${UTOKENIZE} -ss -paraline -lang=fr < $< > $@

periods.detok.utf8: periods.tok.utf8
	${UDETOKENIZE} -lang=fr $< > $@


# Short cut to execute all targets that test skipping sub-tasks in tokenize.pl.
.PHONY: not_option
HYPHENS = hyphens.noss hyphens.notok hyphens.pretok hyphens.noss.notok
PERIODS = periods.noss periods.notok periods.pretok periods.noss.notok
PARAOUT = hyphens.ss.tok.paraline.p hyphens.ss.tok.paraline hyphens.ss.tok.p
not_option: ${HYPHENS} $(addsuffix .utf8, ${HYPHENS})
not_option: ${PERIODS} $(addsuffix .utf8, ${PERIODS})
not_option: ${PARAOUT} $(addsuffix .utf8, ${PARAOUT})

# Verify that the not option targets match the reference.
compare_not_option: $(addprefix compare., ${HYPHENS} $(addsuffix .utf8, ${HYPHENS}))
compare_not_option: $(addprefix compare., ${PERIODS} $(addsuffix .utf8, ${PERIODS}))
compare_not_option: $(addprefix compare., ${PARAOUT} $(addsuffix .utf8, ${PARAOUT}))

# Adding dependencies to french-hyphens.
french-hyphens: compare_not_option

# No sentence splitting.
hyphens.noss: hyphens
	${TOKENIZE} -lang=fr -noss < $< > $@

hyphens.noss.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -noss < $< > $@

periods.noss: periods
	${TOKENIZE} -lang=en -noss < $< > $@

periods.noss.utf8: periods.utf8
	${UTOKENIZE} -lang=en -noss < $< > $@

# No tokenization.
hyphens.notok: hyphens
	${TOKENIZE} -lang=fr -ss -notok < $< > $@

hyphens.notok.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -ss -notok < $< > $@

periods.notok: periods
	${TOKENIZE} -lang=en -ss -paraline -notok < $< > $@

periods.notok.utf8: periods.utf8
	${UTOKENIZE} -lang=en -ss -paraline -notok < $< > $@

# Pre-tokenized.
hyphens.pretok: hyphens
	${TOKENIZE} -lang=fr -noss < $< | ${TOKENIZE} -lang=fr -ss -pretok > $@

hyphens.pretok.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -noss < $< | ${UTOKENIZE} -lang=fr -ss -pretok > $@

periods.pretok: periods
	${TOKENIZE} -lang=en -noss < $< | ${TOKENIZE} -lang=fr -ss -paraline -pretok > $@

periods.pretok.utf8: periods.utf8
	${UTOKENIZE} -lang=en -noss < $< | ${UTOKENIZE} -lang=en -ss -paraline -pretok > $@

# No sentence splitting & no tokenization.
hyphens.noss.notok: hyphens
	${TOKENIZE} -lang=fr -noss -notok < $< > $@

hyphens.noss.notok.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -noss -notok < $< > $@

periods.noss.notok: periods
	${TOKENIZE} -lang=en -noss -notok < $< > $@

periods.noss.notok.utf8: periods.utf8
	${UTOKENIZE} -lang=en -noss -notok < $< > $@

# -p option
hyphens.ss.tok.paraline.p: hyphens
	${TOKENIZE} -lang=fr -ss -paraline -p < $< > $@

hyphens.ss.tok.paraline.p.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -ss -paraline -p < $< > $@

hyphens.ss.tok.paraline: hyphens
	${TOKENIZE} -lang=fr -ss -paraline < $< > $@

hyphens.ss.tok.paraline.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -ss -paraline < $< > $@

hyphens.ss.tok.p: hyphens
	${TOKENIZE} -lang=fr -ss -p < $< > $@

hyphens.ss.tok.p.utf8: hyphens.utf8
	${UTOKENIZE} -lang=fr -ss -p < $< > $@

# Chinese specific.
chinese-punc.detok.000: chinese-punc
	${UDETOKENIZE} < $< > $@

chinese-punc.detok.001: chinese-punc
	${UDETOKENIZE} -stripchinese < $< > $@

chinese-punc.detok.010: chinese-punc
	${UDETOKENIZE} -chinesepunc < $< > $@

chinese-punc.detok.011: chinese-punc
	${UDETOKENIZE} -chinesepunc -stripchinese < $< > $@

chinese-punc.detok.100: chinese-punc
	${UDETOKENIZE} -latin1 < $< > $@

chinese-punc.detok.101: chinese-punc
	${UDETOKENIZE} -latin1 -stripchinese < $< > $@

chinese-punc.detok.110: chinese-punc
	${UDETOKENIZE} -latin1 -chinesepunc < $< > $@

chinese-punc.detok.111: chinese-punc
	${UDETOKENIZE} -latin1 -chinesepunc -stripchinese < $< > $@

# Price specific ($ processing in English).
eng-prices.utf8: eng-prices
	iconv -f iso-8859-1 -t UTF-8 < $< > $@

eng-prices.detok: eng-prices
	${DETOKENIZE} -lang=en $< > $@

eng-prices.detok.utf8: eng-prices.utf8
	${UDETOKENIZE} -lang=en $< > $@


#####################
# Test map-chinese-punct.pl, a separate script just for that purpose

ch-punc-utf8.%: ch-punc-utf8
	map-chinese-punct.pl -$* < $< > $@

#####################
# Make sure the apostrophe from CP1252 and its utf-8 counterpart are
# detokenized correctly
test: test-smart-apos

.PHONY: test-smart-apos
test-smart-apos: compare.smart-apos.detok.utf8 compare.smart-apos.detok \
                 compare.smart-apos.tok.utf8 compare.smart-apos.tok

smart-apos.utf8: smart-apos
	iconv -f cp1252 -t utf-8 < $< > $@

smart-apos.detok.utf8: smart-apos.utf8
	udetokenize.pl -lang=en $< > $@

smart-apos.detok: smart-apos
	detokenize.pl -lang=en $< > $@

smart-apos.tok.utf8: smart-apos.detok.utf8
	utokenize.pl -lang=en -noss $< > $@

smart-apos.tok: smart-apos.detok
	tokenize.pl -lang=en -noss $< > $@


################################################################################
# SPANISH
# These should come back identical after tokenization & detokenization.
test: spanish

.PHONY: spanish
spanish: compare.es.tok compare.es.detok

SOURCE_ES := A Bush, esperanza. \
             ¿Lo mismo Unidos? \
             Si no te gusta la comida, ¿por qué la comes? \
             ¡Qué lástima, estás bien? \
             Gana \$$30.000 por año. \
             Quiero leer \"Romeo y Julieta\". \
             Quiero leer «Romeo y Julieta». \
             \"Crepúsculo\", \"Cien años de soledad\", y \"El zahir\" son libros populares. \
             ¡¿Qué viste?! \
             ¡¡¡Idiota!!! \
             «Antonio me dijo: “Vaya ‘cacharro’ que se ha comprado Julián”».

# This dialog should come back in three sentences.
SOURCE_ES += — ¿Cómo estás? — Muy bien ¿y tú? — Muy bien también.

es.source:
	echo "${SOURCE_ES}" > $@

es.tok: es.source
	${UTOKENIZE} -lang=es -ss < $< > $@

es.detok: es.tok
	${UDETOKENIZE} -lang=es < $< > $@


################################################################################
# DANISH
test: danish

SOURCE_DA := »…« \
             ›…‹ \
				 „…“ \
				 “…” \
				 ‚…‘

.PHONY: danish
danish:  compare.da.tok compare.da.detok

da.source:
	echo "${SOURCE_DA}" > $@

da.tok:  da.source
	utokenize.pl -lang=da -noss < $< > $@

da.detok:  da.tok
	udetokenize.pl -lang=da < $< > $@


################################################################################
# Handling tags
test: tags

.PHONY: tags
tags: compare.xtags.tok compare.xtags.detok

xtags.tok: xtags
	utokenize.pl -noss -xtags < $< > $@

xtags.detok: xtags.tok
	udetokenize.pl < $< > $@

################################################################################
# Detokenizing with brackets of all kinds
test: brackets

.PHONY: brackets
brackets: $(foreach l,en es fr da,compare.brackets.utf8.detok-$l)

brackets.utf8.detok-%: brackets.utf8
	udetokenize.pl -lang=$* $< > $@



################################################################################
# Tokenizing a string with markup with tokenize in xtags or not mode has an uninitialized value.
# Second TU from 12090617-E-20120926_merged_LDV2_CG1_RIASESBRAVD_clean.tmx
# NOTE: the output is missing a "f" the "f" of "of".
test: with_tags
.PHONY: with_tags
with_tags:  with_tags.out
	egrep ' of ' $<
	diff $< ref/$<

with_tags.in:
	echo "<cf font=\"Verdana\" size=\"8\" complexscriptssize=\"8\" asiantextfont=\"Verdana\" bold=\"on\"> Page </cf><field/><cf font=\"Verdana\" size=\"8\" complexscriptssize=\"8\" asiantextfont=\"Verdana\" bold=\"on\"> of </cf><field/>" > $@

with_tags.out:  SHELL = bash
with_tags.out:  with_tags.in
	utokenize.pl -lang=en -noss -xtags $< $@ 2>&1 \
	| { ! egrep 'Use of uninitialized value in addition'; }


# When using sentence splitting the output when tokenized should contain the
# same number of lines as the output not tokenized.
UNITTEST_INPUT := ---. 1983.
# Disabled since notok != tok because tok also collapses --- to --
test: unittest1
.PHONY: unittest1
unittest1: SHELL=bash
unittest1:
	[[ `echo "${UNITTEST_INPUT}" | ${UTOKENIZE} -ss -notok | \wc -l` == `echo "${UNITTEST_INPUT}" | ${UTOKENIZE} -ss | \wc -l` ]] \
	|| ! echo "Doing sentence splitting on a string should produce the same number of output lines in either tokenized mode or not." >&2


################################################################################
# Handling all the newline semantic variants
test: newline
newline: $(foreach s,ss noss,$(foreach p,w wp p pp,compare.paragraph.$s-$p))

paragraph.ss-w: paragraph
	utokenize.pl -lang=en -ss < $< > $@

paragraph.ss-wp: paragraph
	utokenize.pl -lang=en -ss -p < $< > $@

paragraph.ss-p: paragraph
	utokenize.pl -lang=en -ss -paraline < $< > $@

paragraph.ss-pp: paragraph
	utokenize.pl -lang=en -ss -paraline -p < $< > $@

paragraph.noss-w: paragraph
	utokenize.pl -lang=en -noss < $< > $@

paragraph.noss-wp: paragraph
	! utokenize.pl -lang=en -noss -p < $< > $@

paragraph.noss-p: paragraph
	! utokenize.pl -lang=en -noss -paraline < $< > $@

paragraph.noss-pp: paragraph
	! utokenize.pl -lang=en -noss -paraline -p < $< > $@

################################################################################
# Empty paragraph with -ss -notok -p -paraline used to cause error
test: empty.paragraph empty.paragraph.utf8

empty.paragraph:
	echo " " | ${TOKENIZE} -lang=en -ss -notok -p -paraline >& $@
	echo $$'\n' | diff - $@

empty.paragraph.utf8:
	echo " " | ${UTOKENIZE} -lang=en -ss -notok -p -paraline >& $@
	echo $$'\n' | diff - $@

