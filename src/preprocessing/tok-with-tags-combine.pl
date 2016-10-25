#!/usr/bin/env perl

# @file tok-with-tags-combine.pl
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

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 TEXTTOK TAGSIN OUTPUT

  Handle XML tags in tokenizing languages not supported by utokenize.pl, or
  when using an alternative tokenizer.

  tok-with-tags-combine.pl takes the tokenized text and tags streams produced
  by tok-with-tags-split.pl and a third party tokenizer, and combines them into
  the final output.

Arguments:

  TEXTTOK is the TEXTOUT file produced by tok-with-tags-split.pl, as processed
  by your tokenizer.

  TAGSIN is the TAGSOUT file produced by tok-with-tags-split.pl.

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
my ($texttok, $tagsin, $output) = @ARGV;

my $text_id = 0;
open TEXTTOK, "<$texttok"  or die "Error: Cannot open input tokenized-text file $texttok: $!";
open TAGSIN, "<$tagsin"  or die "Error: Cannot open input tags file $tagsin: $!";
open OUT, ">$output" or die "Error: Cannot open output file $output: $!";

while (<TAGSIN>) {
   s/TEXT ID([0-9]+)/
      my $contents = <TEXTTOK>;
      die "Error: Missing line in $texttok for TEXT ID$1" if (!defined $contents);
      chomp $contents;
      $contents =~ s# $##;
      $contents
   /eg;
   s/   */ /g;
   s/ *$//;
   print OUT;
}
if (defined <TEXTTOK>) {
   die "Error: File $texttok too long";
}
