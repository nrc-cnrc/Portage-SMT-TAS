#!/usr/bin/perl -s

# sum.pl
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# George Foster
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use strict;
use warnings;

print STDERR "sum.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

my $HELP = "
sum.pl [in [out]]

Sum a column of numbers.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
my $in = shift || "-";
my $out = shift || "-";
 
open(IN, "<$in") or die "Can't open $in for reading";
open(OUT, ">$out") or die "Can't open $out for writing";

my $sum = 0.0;
while (<IN>) {
    $sum += $_;
}

print OUT "$sum\n";
