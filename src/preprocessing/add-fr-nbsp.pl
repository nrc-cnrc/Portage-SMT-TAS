#!/usr/bin/env perl
# @file add-fr-nbsp.pl
# @brief Component for postprocess_plugin: add non-breaking spaces in French
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

use strict;
use warnings;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils", $bin_path;
   }
}
use portage_utils;
printCopyright(2015);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use encoding "UTF-8";
use utf8; # This script uses literal utf-8 characters.
use ULexiTools;
my $tag_re = get_tag_re();

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 < INPUT > OUTPUT

  Add non-breaking spaces in French output without breaking XML tags.

  Applies these rules:
   - non-breaking space before colon, closing French quotes, percent and
     currencies: :, », %, \$, €, ¥, £
   - non-breaking space after opening French quotes: «
   - non-breaking space between a digit and a one or two-letter symbol, e.g.,
     V, cm, ml.

  This script is tag-aware: the rules are only applies to text outside tags,
  never within the tags themselves.

Options:

  -nosymbols    Skip to third rule (nbsp between digit and symbol)

  -h(elp)       print this help message
";
   exit 1;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   nosymbols   => \my $nosymbols,
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
) or usage;

my $nbsp = " "; # There is a literal non-break space, \xA0, in the quotes here
while (<>) {
   chomp;
   while (/(.*?)($tag_re|$)/go) {
      my $text = $1;
      my $tag = $2;

      # Make the space before :, », %, etc, non-breaking
      $text =~ s/ (:|»|%|\$|€|¥|£)/$nbsp$1/g;
      # Make the space after « non-breaking
      $text =~ s/(«) /$1$nbsp/g;
      # Make the space between a number and a 1 or 2-letter symbol non-breaking
      $text =~ s/(\d) ([a-zA-Z]{1,2})\b/$1$nbsp$2/g unless $nosymbols;

      print $text;
      print $tag;
   }
   print "\n";
}
