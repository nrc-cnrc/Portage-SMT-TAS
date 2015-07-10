#!/usr/bin/env perl

# @file mark-french-numbers.pl
# @brief Mark up French numbers with their English equivalents.
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

use strict;
use warnings;
use 5.10.0;

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
printCopyright 2015;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] source_lang < IN > OUT

  The following sample code maps numbers of the following forms, from
  French to English:
   - 420 000 -> 420,000
   - 14,5 -> 14.5
   - 12 345 678,90 -> 12,345,678.90
   - An optional + or - in front is allowed and preserved
   - The number has to cover one or more whole tokens, so that we don't
     accidentally grab parts of longer codes
   - The number has to be in groups of three: 4-digit years will not be
     touched.
   - Or it has to have a decimal comma and up to 5 digits on either side:
     12345,678 -> 12345.678
   - This code is aware of xmlish markup done prior, i.e. fixed terms, and
     protects it.

   source_lang  source language of the input [must be fr].

Options:

  -h(elp)       print this help message
";
   exit 1;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
GetOptions(
   help        => sub { usage },
) or usage "Error: Invalid option(s).";


my $SOURCE_LANGUAGE = shift or die "Error: Missing language code argument";
die "Error: This number parser only works with English input" unless ($SOURCE_LANGUAGE eq 'fr');


while (<>) {
   # Turn off warnings because all unmatched parts below cause warnings about
   # using unitialized variables.
   no warnings;

   # Don't mark number that have previously been marked as fixed terms.
   # TODO $8 == (?=\ |$) which is a Positive lookahead thus it should always be undefined?
   # To illustrate
   # perl -e 'print "|$&|$1|" if ("ab" =~ m/a(?=b)/)'
   # > |a||
   s/
      (?<TAG><(FT)[^>]+>(?:.*?)<\/\2>)      # Skip tags
     |
      (?<PRE>^|\ )                          # space or start of line marks token boundary
         (?<WHOLE>[-+]?\d{1,3}(?:\ \d{3})*) # Whole part, with optional sign
         (?<FRAC>,(?:\d{3}\ )*\d{1,3})?     # fractional part
      (?=\ |$)                              # space or end of line marks token boundary
     |
      (?<PRE>^|\ )                          # space or start of line marks token boundary
         (?<WHOLE>[-+]?\d{1,5})             # 1 to 5 digit whole part, with optional sign, no spaces
         (?<FRAC>,\d{1,5})                  # 1 to 5 digit fraction, no spaces
      (?=\ |$)                              # space or end of line marks token boundary
   /
      if (defined($+{TAG})) {
         $+{TAG};
      } else {
         $+{PRE} .
         "<NUMFREN target=\"" .
         do { my $whole = $+{WHOLE}; $whole =~ s# #,#g; $whole } .
         do { my $frac = $+{FRAC}; $frac =~ s#^,#.#; $frac =~ s# #,#g; $frac } .
         "\">$+{WHOLE}$+{FRAC}<\/NUMFREN>"
      }
   /exg;
   print;
}
