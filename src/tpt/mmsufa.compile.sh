#!/bin/bash
# This file is derivative work from Ulrich Germann's Tightly Packed Tries
# package (TPTs and related software).
#
# Original Copyright:
# Copyright 2005-2009 Ulrich Germann; all rights reserved.
# Under licence to NRC.
#
# Copyright for modifications:
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008-2010, Her Majesty in Right of Canada



# simple script for compiling a text file into a suffix array
# three files are produced: 
# $2.tdx: TokenIndex (vocabulary file; memory-mapped)
# $2.mct: memory-mapped corpus track 
# $2.msa: memory-mapped suffix array

in=$1
out=$2

zcat -f $in | vocab.build --tdx $out.tdx
zcat -f $in | mmctrack.build $out.tdx $out.mct
mmsufa.build $out.mct $out.msa

