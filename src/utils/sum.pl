#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;


# @file sum.pl 
# @brief Sum a column of numbers.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils";
   }
}
use portage_utils;
# Sum is used way too much, and nearly exclussively internally - don't print
# the Copyright
#printCopyright "sum.pl", 2005;
#$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP = "
sum.pl [-namr] [in [out]]

Sum a column of numbers.

Options:
-n  Normalize numbers by dividing by their sum, and print results.
-a  Print average instead of sum.
-m  Print max instead of sum.
-r  Use reciprocals of numbers.
-l  Input is a list rather than a column: add all whitespace-separated numbers.
-v  Show some verbose info.

";

our ($help, $h, $n, $r, $a, $m, $l, $v);

if ($help || $h) {
   printCopyright "sum.pl", 2005;
   print $HELP;
   exit 0;
}
 
my $in = shift || "-";
my $out = shift || "-";

open(IN, "<$in") or die "Error: Can't open $in for reading";
open(OUT, ">$out") or die "Error: Can't open $out for writing";

my @vals;

my $sum = 0.0;
my $count = 0;
my $max;
while (<IN>) {
   no warnings; # Just do the best we can with the user input; don't complain!

   foreach ($l ? split : $_) {
      my $x = $r ? 1.0 / $_ : $_;

      $sum += $x;
      if ($n) {push @vals, ($x);}
      if ($m) { if (!defined $max || $x > $max) { $max = $x; } }

      ++$count;
   }
}

if ($n) {
   while (my $x = shift @vals) {print OUT $x/$sum, "\n";}
} elsif ($m) {
   if ( defined $max ) {
      print OUT "$max\n";
   } else {
      print OUT "0\n";
   }
} else {
   if ($v) {print OUT "N=$count  "}
   if ($a && $count) {$sum /= $count}
   print OUT "$sum\n";
}
