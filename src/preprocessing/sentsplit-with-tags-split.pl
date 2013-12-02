#!/usr/bin/env perl
# @file sentsplit-with-tags-split.pl
# @brief Sent split text with an external sentence splitter, while handling tags (part 1)
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

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
printCopyright(2013);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use ULexiTools qw/get_tag_re/;
my $tag_re = get_tag_re();

my $tag_placeholder = "__PORTAGE_XTAG_PLACEHOLDER__";

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 INPUT TEXTOUT TAGSOUT

  Handle XML tags in splitting sentences for languages not supported by
  utokenize.pl, or when using an alternative sentence splitter.

  sentsplit-with-tags-split.pl takes the input text with its tags and splits it
  in two streams for separate processing:
   - The text stream has the tags replaces by $tag_placeholder,
     with the results saved to TEXTOUT.
   - The tag stream TAGSOUT will have one tag per line, giving the full
     contents of each tag for reinsertion in the final output.

  You should pipe TEXTOUT through your language-specific sentence splitter to
  produce TEXTSS, and then call:
     sentsplit-with-tags-combine.pl TEXTSS TAGSOUT OUTPUT
  OUTPUT will then be your sentence-split text with tags correctly handled.

Arguments:

  INPUT: file to sentence split, with tags mixed in the stream

  TEXTOUT: output file that will contain only text to sentence split

  TAGSOUT: output file for the tags, with one tag per line

Options:

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
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
) or usage;

3 == @ARGV or usage "You must provide exactly three file names as arguments.";
my ($input, $textout, $tagsout) = @ARGV;

my $text_id = 0;
open TEXT, ">$textout" or die "Cannot open text output file $textout: $!";
open TAGS, ">$tagsout" or die "Cannot open tags output file $tagsout: $!";
open INPUT, "<$input"  or die "Cannot open input file $input: $!";
while (<INPUT>) {
   chomp;
   my $tags_to_tok = "";
   while (/(.*?)($tag_re|$)/go) {
      my $text = $1;
      my $tag = $2;
      print TEXT $text;
     last if ($tag eq "");
      print TEXT $tag_placeholder;
      print TAGS "$tag\n";
   }
   print TEXT "\n";
}
close TEXT or die "Error closing text output file $textout: $!";
close TAGS or die "Error closing tags output file $tagsout: $!";
close INPUT or die "Error closing input file $input: $!";
