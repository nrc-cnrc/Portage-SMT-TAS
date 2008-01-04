#!/usr/bin/perl -s

# ngrams.pl
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#

# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "ngrams.pl, NRC-CNRC, (c) 2005 - 2008, Her Majesty in Right of Canada\n";

my $HELP = "
ngrams.pl n [in [out]]

Write all ngrams (tokens, not types) occuring in tokenized ospl input text.
Just the ngrams; no fancy stuff with BOS/EOS markers.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}

my $n = shift or die "missing n argument!\n$HELP";
 
my $in = shift || "-";
my $out = shift || "-";
 
open(IN, "<$in") or die "Can't open $in for reading\n";
open(OUT, ">$out") or die "Can't open $out for writing\n";

while (<IN>) {
   my @words = split;
   for (my $i = 0; $i <= $#words+1 - $n; ++$i) {
      for (my $j = 0; $j < $n; ++$j) {
          print OUT $words[$i+$j], ($j < $n-1 ? " " : "\n");
      }
   }
}
