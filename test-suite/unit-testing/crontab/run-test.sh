#!/bin/bash
# @file run-test.sh
# @brief Run this test suite, with a non-zero exit status if it fails
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2018, Sa Majeste la Reine du Chef du Canada
# Copyright 2018, Her Majesty in Right of Canada


make clean
make all -j 1
