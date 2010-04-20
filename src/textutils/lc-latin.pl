#!/usr/bin/perl -s

# @file lc-latin1.pl 
# @brief Lowercase mapping for iso-latin1.
# 
# @author George Foster
# 
# COMMENTS: 
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada


require 5.004;
use locale;
use POSIX qw(locale_h);
setlocale(LC_CTYPE, "fr_CA.iso88591");

$HELP = "
lc-latin.pl [in [out]]

Lowercase mapping for iso-latin1. This works regardless of how you have
your locale set up. Not really portable, though!

";

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
$in = shift || "-";
$out = shift || "-";
 
if (!open(IN, "<$in")) {die "Can't open $in for reading";}
if (!open(OUT, ">$out")) {die "Can't open $out for writing";}

# Enable immediate flush even when piping
select(OUT); $| = 1;

while (<IN>) {print OUT lc;}
