#!/bin/bash

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

