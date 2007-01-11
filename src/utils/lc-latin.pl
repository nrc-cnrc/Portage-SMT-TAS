#!/usr/bin/perl -s

# lc-latin1.pl Lowercase mapping for iso-latin1
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

print STDERR "lc-latin1.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

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
 
if (!open(IN, "<$in")) {die "Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Can't open $out for reading";}

# Enable immediate flush when piping
select(OUT); $| = 1;

while (<IN>) {print OUT lc;}
