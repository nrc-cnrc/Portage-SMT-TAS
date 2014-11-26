#!/usr/bin/env perl
# @file convertEnglishNumber.pl
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada
#

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
printCopyright 2014;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] source_lang < IN > OUT

  The following sample code maps numbers of the following forms, from
  English to French:
   - 420K -> 420 000
   - 14.5M -> 14,5 millions
   - 12,345,678.90 -> 12 345,678 90
   - An optional + or - in front is allowed and preserved
   - The number has to be the whole token, so that we don't
     accidentally grab longer codes
   - The number has to be in groups of three: 4-digit years will not be
     touched.
   - This code is aware of xmlish markup done prior i.e. fixed terms

   source_lang  source language of the input [must be en].

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
) or usage;


my $SOURCE_LANGUAGE = shift or die "Missing language code argument";
die "This number parser only works with English input" unless ($SOURCE_LANGUAGE eq 'en');


while (<>) {
   # Don't mark number that have previously been marked as fixed terms.
   # TODO $8 == (?=\ |$) which is a Positive lookahead thus it should always be undefined?
   # To illustrate
   # perl -e 'print "|$&|$1|" if ("ab" =~ m/a(?=b)/)'
   # > |a||
   s/(?<TAG><(FT)[^>]+>(?:.*?)<\/\2>)|(^|\ )([-+]?\d{1,3}(?:,\d{3})*)(k|\.(?:\d{3},)*\d{1,3}|(\.\d{1,3})?(m))?(?=\ |$)/
      if (defined($+{TAG})) {
         $+{TAG};
      }
      else {
         $3 .
         "<NUMK target=\"" .
         do { my $num = $4; $num =~ s#,# #g; $num } .
         ($5 eq "k"
            ? " 000"
            : ($7 eq "m"
               ? do { my $num = $6; $num =~ s#^\.#,#; $num } . " millions"
               : do { my $num = $5; $num =~ s#,# #g; $num =~ s#^\.#,#; $num }
              )
         ) .
         "\">$4$5<\/NUMK>"
      }
    /exg;
    print;
}
