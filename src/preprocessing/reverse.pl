#!/usr/bin/perl -sw

# @file reverse.pl 
# @brief Reverses the words in each line of input.
# 
# @author Aaron Tikuisis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;

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
printCopyright("reverse.pl", 2005);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP =
"Usage: reverse.pl [infile [outfile]]

  Reverses the words in each line of input, outputting the result.  Words in a
  line must be delimited by spaces (other forms of whitespace are counted as
  part of a word).

Options:
  infile   The input file; if not specified, standard input is used.
  outfile  The output file; if not specified, standard output is used.

";

our ($help, $h);

if ($help || $h)
{
    print $HELP;
    exit 0;
} # if

my $in = shift || "-";
my $out = shift || "-";

open(IN, "<$in") || die "Error: Can't open $in for reading";
open(OUT, ">$out") || die "Error: Can't open $out for writing";

# Enable immediate flush when piping
select(OUT); $| = 1;

while (my $line = <IN>)
{
    $line =~ s/\n//;
    my @words = split(/ /, $line);
    @words = reverse @words;
    print OUT "@words\n";
} # while
