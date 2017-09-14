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

html/%.html: asciidoc/%.adoc asciidoc/header.adoc asciidoc/footer.adoc
	asciidoctor --doctype=book -a showtitle=true -a icons=font@ -a source-highlighter=highlight.js -a platform=opal -a platform-opal=true -a env=browser -a env-browser=true -a idprefix=true -a idseparator=- -a setanchors=true -a toc=macro -o $@ $<
#	asciidoc -a data-uri -a icons -a toc -a toclevels=3 -a max-width=55em -o $@ $<
#	asciidoc --doctype=book -a icons -a toc -a toc.max.depth=4 -a generate.section.toc.level=4 -a max-width=55em -o $@ $<

