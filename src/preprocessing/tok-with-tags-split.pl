#!/usr/bin/env perl

# @file tok-with-tags-split.pl
# @brief Tokenize text with an external tokenizer, while handling tags (part 1)
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

use ULexiTools;
my $tag_re = get_tag_re();
setTokenizationLang("en"); # for tokenizing tags themselves, not real text

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 INPUT TEXTOUT TAGSOUT

  Handle XML tags in tokenizing languages not supported by utokenize.pl, or
  when using an alternative tokenizer.

  tok-with-tags-split.pl takes the input text with its tags and splits it in
  two streams for separate tokenization:
   - The tag stream is tokenized internally using ULexiTools directly, with the
     results saved in TAGSOUT.  By tokenizing the tag stream, we mean adding
     spaces before or after tags, as required depending on the sequence of
     tags, and which ones are open, close or stand-alone tags.
   - The text stream contains the text between tags and is saved to TEXTOUT.

  You should pipe TEXTOUT through your language-specific tokenizer to produce
  TEXTTOK, and then call:
     tok-with-tags-combine.pl TEXTTOK TAGSOUT OUTPUT
  OUTPUT will then be your tokenized text with tags correctly handled.

Arguments:

  INPUT: file to tokenize, with tags mixed in the stream

  TEXTOUT: output file that will contain only text to tokenize

  TAGSOUT: output file for the tags, with spaces already added where necessary.

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
) or usage "Error: Invalid option(s).";

3 == @ARGV or usage "Error: You must provide exactly three file names as arguments.";
my ($input, $textout, $tagsout) = @ARGV;

my $text_id = 0;
open TEXT, ">$textout" or die "Error: Cannot open text output file $textout: $!";
open TAGS, ">$tagsout" or die "Error: Cannot open tags output file $tagsout: $!";
open INPUT, "<$input"  or die "Error: Cannot open input file $input: $!";
while (<INPUT>) {
   chomp;
   my $tags_to_tok = "";
   while (/(.*?)( *$tag_re *|$)/go) {
      my $text = $1;
      my $tag = $2;
      if ($text !~ /^ *$/) {
         ++$text_id;
         #print "TEXT ID$text_id";
         $tags_to_tok .= "TEXT ID$text_id";
         print TEXT $text, "\n";
      }
    last if ($tag eq "");
      #print $tag;
      $tags_to_tok .= $tag;
   }
   #print "\n";

   # Call tokenizer methods directly: this code inserts spaces where needed
   # around the tags.
   my @token_positions = tokenize($tags_to_tok, 0, 1); # $pretok=0, $xtags=1
   my $tags_toked = "";
   for (my $i = 0; $i < $#token_positions; $i += 2) {
      $tags_toked .= " " if ($i > 0);
      $tags_toked .= get_collapse_token($tags_to_tok, $i, @token_positions, 0);
   }
   print TAGS $tags_toked, "\n";
}
close TEXT or die "Error: Error closing text output file $textout: $!";
close TAGS or die "Error: Error closing tags output file $tagsout: $!";
close INPUT or die "Error: Error closing input file $input: $!";
