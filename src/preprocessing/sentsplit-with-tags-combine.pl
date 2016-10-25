#!/usr/bin/env perl

# @file sentsplit-with-tags-combine.pl
# @brief Tokenize text with an external tokenizer, while handling tags (part 2)
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

my $tag_placeholder = "__PORTAGE_XTAG_PLACEHOLDER__";

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 TEXTSS TAGSIN OUTPUT

  Handle XML tags in splitting sentences for languages not supported by
  utokenize.pl, or when using an alternative sentence splitter.

  sentsplit-with-tags-combine.pl takes the sentence-split text and tags streams
  produced by sentsplit-with-tags-split.pl and a third party sentence splitter,
  and combines them into the final output.

Arguments:

  TEXTSS is the TEXTOUT file produced by sentsplit-with-tags-split.pl, as processed
  by your tokenizer.

  TAGSIN is the TAGSOUT file produced by sentsplit-with-tags-split.pl.

  OUTPUT is the final output.

Options:

  -h(elp)       print this help message
";
   exit @_ ? 1 : 0;
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
) or usage "Error: Invalid option(s).";

3 == @ARGV or usage "Error: You must provide exactly three file names as arguments.";
my ($textss, $tagsin, $output) = @ARGV;

my $text_id = 0;
open TEXTSS, "<$textss"  or die "Error: Cannot open input sentence-split-text file $textss: $!";
open TAGSIN, "<$tagsin"  or die "Error: Cannot open input tags file $tagsin: $!";
open OUT, ">$output" or die "Error: Cannot open output file $output: $!";

while (<TEXTSS>) {
   s/$tag_placeholder/
      my $tag = <TAGSIN>;
      die "Error: File $tagsin too short" if (!defined $tag);
      chomp $tag;
      $tag
   /ego;
   print OUT;
}
if (defined <TAGSIN>) {
   die "Error: File $tagsin too long";
}
