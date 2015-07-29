#!/usr/bin/env perl

# @file mark-numbers-en2fr.pl
# @brief Mark up English numbers with their French equivalents.
#
# @author Samuel Larkin; ugly RE by Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

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
printCopyright 2014;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] source_lang < IN > OUT

  Mark numbers for translation from English to French according to the
  following rules:
   - 420K -> 420 000
   - 14.5M -> 14,5 millions
   - 12,345,678.90 -> 12 345 678,90
   - An optional + or - in front is allowed and preserved
   - The number has to be the whole token, so that we don't
     accidentally grab longer codes
   - The number has to be in groups of three: 4-digit years will not be
     touched.
   - This code is aware of xmlish markup done prior, i.e. fixed terms,
     and protects it from unwanted modification.

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
) or usage "Error: Invalid option(s).";


my $SOURCE_LANGUAGE = (shift || "en");
die "Error: This number parser only works with English input" unless ($SOURCE_LANGUAGE eq 'en');


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
      (?<TAG><(FT)[^>]+>(?:.*?)<\/\2>)
     |
      (?<PRE>^|\ )
         (?<DOLLAR>\$\ )?
         (?<WHOLE>[-+]?\d{1,3}(?:,\d{3})*)
         (?<SUFF>k|\.(?:\d{3},)*\d{1,3}|(?<MFRAC>\.\d{1,3})?(?<M>m|\ million))?
      (?=\ |$)
   /
      if (defined($+{TAG})) {
         $+{TAG};
      } elsif (!defined $+{DOLLAR} && !defined $+{SUFF} && length($+{WHOLE}) <= 4) {
         "$+{PRE}$+{WHOLE}"
      } else {
         $+{PRE} .
         "<NUMK target=\"" .
         do { my $num = $+{WHOLE}; $num =~ s#,# #g; $num } .
         ($+{SUFF} eq "k"
            ? " 000"
            : ($+{M} ne ""
               ? do { my $num = $+{MFRAC}; $num =~ s#^\.#,#; $num } . " million" . ($+{WHOLE} eq "1" ? "" : "s")
               : do { my $num = $+{SUFF}; $num =~ s#,# #g; $num =~ s#^\.#,#; $num }
              )
         ) .
         ($+{DOLLAR} eq "\$ " ? ($+{M} ne "" ? " de dollars" : " \$") : "") .
         "\">$+{DOLLAR}$+{WHOLE}$+{SUFF}<\/NUMK>"
      }
   /exg;
   print;
}