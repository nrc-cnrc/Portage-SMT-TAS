#!/usr/bin/perl -s

# @file sum.pl 
# @brief Sum a column of numbers.
# 
# @author George Foster
# 
# COMMENTS: 
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "sum.pl, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada\n";

my $HELP = "
sum.pl [-nr] [in [out]]

Sum a column of numbers.

Options:
-n  Normalize numbers by dividing by their sum, and print results.
-a  Print average of instead of sum.
-r  Use reciprocals of numbers.

";

our ($help, $h, $n, $r, $a);

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
my $in = shift || "-";
my $out = shift || "-";

open(IN, "<$in") or die "Can't open $in for reading";
open(OUT, ">$out") or die "Can't open $out for writing";

my @vals;

my $sum = 0.0;
my $count = 0;
while (<IN>) {
    my $x = $r ? 1.0 / $_ : $_;

    $sum += $x;
    if ($n) {push @vals, ($x);}
    ++$count;
}

if ($n) {
   while (my $x = shift @vals) {print OUT $x/$sum, "\n";}
} else {
   if ($a && $count) {$sum /= $count}
   print OUT "$sum\n";
}
