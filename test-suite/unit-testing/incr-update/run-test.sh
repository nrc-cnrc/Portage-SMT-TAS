#!/bin/bash
# run-test.sh - Run this test suite, with a non-zero exit status if it fails
#
# PROGRAMMER: Eric Joanis and Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

make clean
make all -j 2
