#!/usr/bin/perl -s

# @file ngrams.pl 
# @brief Write all ngrams (tokens, not types) occuring in tokenized ospl input text.
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
printCopyright "ngrams.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


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
