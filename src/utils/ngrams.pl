#!/usr/bin/perl -s

# ngrams.pl
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

print STDERR "ngrams.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

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
