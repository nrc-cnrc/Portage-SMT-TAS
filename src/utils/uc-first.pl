#!/usr/bin/perl -s

# uc-first.pl 1st-letter per line uppercase mapping for iso-latin1
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

require 5.004;
use locale;
use POSIX qw(locale_h);
setlocale(LC_CTYPE, "fr_CA.iso88591");

print STDERR "uc-first.pl, Copyright (c) 2005 - 2006, Sa Majeste la Reine du Chef du Canada / Her Majesty in Right of Canada\n";

$HELP = "
uc-first.pl [in [out]]

1st-letter uppercase mapping for iso-latin1. This works regardless of how you
have your locale set up. Not really portable, though!

";

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
$in = shift || "-";
$out = shift || "-";
 
if (!open(IN, "<$in")) {die "Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Can't open $out for reading";}

# Enable immediate flush when piping
select(OUT); $| = 1;

while (<IN>) {print OUT ucfirst;}
