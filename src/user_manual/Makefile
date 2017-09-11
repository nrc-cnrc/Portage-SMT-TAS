# @file Makefile
#
# @brief Generation of the user manual
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada
# Copyright 2017, Her Majesty in Right of Canada

BASELIST:=$(filter-out footer header,$(basename $(notdir $(wildcard asciidoc/*.adoc))))
HTMLLIST:=$(addprefix html/,$(addsuffix .html,$(BASELIST)))
#$(info $(HTMLLIST))

include ../build/Makefile.incl

docs: $(HTMLLIST)

html/%.html: asciidoc/%.adoc
	asciidoc -a data-uri -a icons -a toc -a max-width=55em -o $@ $<

