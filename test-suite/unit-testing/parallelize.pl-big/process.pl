#!/usr/bin/perl -sw

# @file process.pl 
# @brief Simple procedure to give to parallelize.pl in unit-tests.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

use strict;

# Usage $0 f1.in f1.out f2.in.gz f2.out.gz

my $in = $ARGV[0];
my $out = $ARGV[1];
open(IN, "$in") or die "Error: Can't open $in";
open(OUT, ">$out") or die "Error: Can't open $out";
while (<IN>) {
   tr/[1,2,3,4,5,6,7,8,9]/[a,b,c,d,e,f,g,h,i]/;
   print OUT $_;
}
close(OUT) or die "Error: $out wasn't entirely processed.";
close(IN) or die "Error: $in wasn't entirely processed.";


$in = $ARGV[2];
$out = $ARGV[3];
open(IN, "$in") or die "Error: Can't open $in";
open(OUT, ">$out") or die "Error: Can't open $out";
while (<IN>) {
   tr/[1,2,3,4,5,6,7,8,9]/[A,B,C,D,E,F,G,H,I]/;
   print OUT $_;
}
close(OUT) or die "Error: $out wasn't entirely processed.";
close(IN) or die "Error: $in wasn't entirely processed.";
