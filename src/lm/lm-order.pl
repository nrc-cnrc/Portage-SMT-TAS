#!/usr/bin/env perl

# @file lm-order.pl
# @brief Determine the order of an ARPA format LM file.
# 
# @author Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

if ($#ARGV >= 0 && $ARGV[0] eq "-h") {
   print STDERR "Usage: lm-order.pl <LM file>

  Display the order of a language model file in ARPA format.

";
   exit;
}

open FILE, "gzip -cqfd $ARGV[0]|" or die "lm-order.pl: Can't open $ARGV[0]: $!\n";
my $order = 0;
while (<FILE>) {
   last if /^\\1-grams:/;
   if ( /\s*ngram\s*(\d+)\s*=\s*(\d+)/ ) {
      ++$order;
      warn "Expected order $order ngram count, got: $_" if $1 != $order;
   }
}
print $order, "\n";
