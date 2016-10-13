#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

# @file Makefile
# @brief Tests to fix misc problems with canoe
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

TEMP_FILES=
TEMP_DIRS=
include ../Makefile.incl

all: input-problems


# Reported by George on April 5, 2016:
# echo '<R english="a>">A</R>' | canoe -f canoe.ini
# Admittedly an extremely rare input, but figured you might still want to know. 
# Other unescaped special characters inside the ="..." do the trick as well.
# George
input-problems: infinite-loop bad1 bad2

infinite-loop:
	! echo '<R english="a>">A</R>' | canoe -f ../canoe-daemon/canoe.ini

bad1:
	! echo '<R class>="asdf" english="a">asdf</R>' | canoe -f ../canoe-daemon/canoe.ini

bad2:
	! echo '<R english>="a">asdf</R>' | canoe -f ../canoe-daemon/canoe.ini

