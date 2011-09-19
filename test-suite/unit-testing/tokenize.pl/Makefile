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


.PHONY: all clean french-hyphens dots ch-punc

all: test
	@echo All tests PASSED

.PHONY: test
test: display.accent \
     $(addprefix compare., accent.tok accent.utf8 accent.utf8.tok) \
     $(addprefix display., accent.tok accent.utf8 accent.utf8.tok) \
     french-hyphens dots ch-punc en-prices


french-hyphens: compare.hyphens.tok compare.hyphens.tok.utf8 \
                compare.hyphens.detok compare.hyphens.detok.utf8

dots: compare.periods.tok compare.periods.tok.utf8 \
      compare.periods.detok compare.periods.detok.utf8

ch-punc: $(foreach EXT, 000 001 010 011 100 101 110 111, compare.chinese-punc.detok.$(EXT))

.PHONY: en-prices
en-prices: compare.eng-prices.detok compare.eng-prices.detok.utf8

clean:
	${RM} accent.tok accent.utf8 accent.utf8.tok 
	${RM} hyphens.utf8 hyphens.{tok,detok}{,.utf8}
	${RM} periods.utf8 periods.{tok,detok}{,.utf8}
	${RM} ${HYPHENS} $(addsuffix .utf8, ${HYPHENS})
	${RM} ${PERIODS} $(addsuffix .utf8, ${PERIODS})
	${RM} chinese-punc.detok.[01][01][01]
	${RM} eng-prices.utf8 eng-prices.detok{,.utf8}

accent.utf8: accent
	iconv -f iso-8859-1 -t UTF-8 < $< > $@

accent.tok: accent
	tokenize.pl -ss $< $@

accent.utf8.tok: accent.utf8
	utokenize.pl -ss $< $@

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
	tokenize.pl -lang=fr -ss $< > $@

hyphens.detok: hyphens.tok
	detokenize.pl -lang=fr $< > $@

hyphens.tok.utf8: hyphens.utf8
	utokenize.pl -ss -lang=fr < $< > $@

hyphens.detok.utf8: hyphens.tok.utf8
	udetokenize.pl -lang=fr $< > $@

# periods specific.
periods.utf8: periods
	iconv -f cp1252 -t utf-8 < $< > $@

periods.tok: periods
	tokenize.pl -lang=fr -ss -paraline $< > $@

periods.detok: periods.tok
	detokenize.pl -lang=fr $< > $@

periods.tok.utf8: periods.utf8
	utokenize.pl -ss -paraline -lang=fr < $< > $@

periods.detok.utf8: periods.tok.utf8
	udetokenize.pl -lang=fr $< > $@


# Short cut to execute all targets that test skipping sub-tasks in tokenize.pl.
.PHONY: not_option
HYPHENS = hyphens.noss hyphens.notok hyphens.pretok hyphens.noss.notok
PERIODS = periods.noss periods.notok periods.pretok periods.noss.notok
not_option: ${HYPHENS} $(addsuffix .utf8, ${HYPHENS})
not_option: ${PERIODS} $(addsuffix .utf8, ${PERIODS})

# Verify that the not option targets match the reference.
compare_not_option: $(addprefix compare., ${HYPHENS} $(addsuffix .utf8, ${HYPHENS}))
compare_not_option: $(addprefix compare., ${PERIODS} $(addsuffix .utf8, ${PERIODS}))

# Adding dependencies to french-hyphens.
french-hyphens: compare_not_option

# No sentence splitting.
hyphens.noss: hyphens
	tokenize.pl -lang=fr -noss < $< > $@

hyphens.noss.utf8: hyphens.utf8
	utokenize.pl -lang=fr -noss < $< > $@

periods.noss: periods
	tokenize.pl -lang=en -noss < $< > $@

periods.noss.utf8: periods.utf8
	utokenize.pl -lang=en -noss < $< > $@

# No tokenization.
hyphens.notok: hyphens
	tokenize.pl -lang=fr -ss -notok < $< > $@

hyphens.notok.utf8: hyphens.utf8
	utokenize.pl -lang=fr -ss -notok < $< > $@

periods.notok: periods
	tokenize.pl -lang=en -ss -paraline -notok < $< > $@

periods.notok.utf8: periods.utf8
	utokenize.pl -lang=en -ss -paraline -notok < $< > $@

# Pre-tokenized.
hyphens.pretok: hyphens
	tokenize.pl -lang=fr -noss < $< | tokenize.pl -lang=fr -ss -pretok > $@

hyphens.pretok.utf8: hyphens.utf8
	utokenize.pl -lang=fr -noss < $< | utokenize.pl -lang=fr -ss -pretok > $@

periods.pretok: periods
	tokenize.pl -lang=en -noss < $< | tokenize.pl -lang=fr -ss -paraline -pretok > $@

periods.pretok.utf8: periods.utf8
	utokenize.pl -lang=en -noss < $< | utokenize.pl -lang=en -ss -paraline -pretok > $@

# No sentence splitting & no tokenization.
hyphens.noss.notok: hyphens
	tokenize.pl -lang=fr -noss -notok < $< > $@

hyphens.noss.notok.utf8: hyphens.utf8
	utokenize.pl -lang=fr -noss -notok < $< > $@

periods.noss.notok: periods
	tokenize.pl -lang=en -noss -notok < $< > $@

periods.noss.notok.utf8: periods.utf8
	utokenize.pl -lang=en -noss -notok < $< > $@


# Chinese specific.
chinese-punc.detok.000: chinese-punc
	udetokenize.pl < $< > $@

chinese-punc.detok.001: chinese-punc
	udetokenize.pl -stripchinese < $< > $@

chinese-punc.detok.010: chinese-punc
	udetokenize.pl -chinesepunc < $< > $@

chinese-punc.detok.011: chinese-punc
	udetokenize.pl -chinesepunc -stripchinese < $< > $@

chinese-punc.detok.100: chinese-punc
	udetokenize.pl -latin1 < $< > $@

chinese-punc.detok.101: chinese-punc
	udetokenize.pl -latin1 -stripchinese < $< > $@

chinese-punc.detok.110: chinese-punc
	udetokenize.pl -latin1 -chinesepunc < $< > $@

chinese-punc.detok.111: chinese-punc
	udetokenize.pl -latin1 -chinesepunc -stripchinese < $< > $@
	
# Price specific ($ processing in English).
eng-prices.utf8: eng-prices
	iconv -f iso-8859-1 -t UTF-8 < $< > $@

eng-prices.detok: eng-prices
	detokenize.pl -lang=en $< > $@
	
eng-prices.detok.utf8: eng-prices.utf8
	udetokenize.pl -lang=en $< > $@



################################################################################
# SPANISH
# These should come back identical after tokenization & detokenization.
test: spanish

clean: clean.spanish
.PHONY: clean.spanish
clean.spanish:
	${RM} es.tok es.detok es.source

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
	utokenize.pl -lang=es -ss < $< | tee $@

es.detok: es.tok
	udetokenize.pl -lang=es < $< | tee $@

