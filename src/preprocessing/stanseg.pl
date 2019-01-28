#!/usr/bin/env perl

# @file stanseg.pl
# @brief Wrapper for the Stanford Segmenter
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2019, Sa Majeste la Reine du Chef du Canada /
# Copyright 2019, Her Majesty in Right of Canada

use utf8;
use strict;
use warnings;
use 5.014;

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
printCopyright(2019);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use ULexiTools;
setTokenizationLang("en"); # for tokenizing tags themselves, not real text

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [-m] [-w] [infile [outfile]]

  Wrapper around the Stanford Segmenter that processes non-arabic text and
  numbers in a way that Portage needs.

  Input should be one-sentence-per-line. (Use sentsplit_plugin ar if necessary.)
  Output will be line-aligned to the input.

Options:

  -m            xmlishify hashtags
  -w(aw)        remove waw prefix at the beginning of the sentence
  -t(hreads) T  Run the Stanford Segmenter with T threads and T GB of RAM

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
   debug       => \my $debug,

   m           => \my $xmlishify_hashtags,
   waw         => \my $remove_waw,
   "threads=i" => \my $threads,
) or usage "Error: Invalid option(s).";

$xmlishify_hashtags = 0 unless defined $xmlishify_hashtags;
$remove_waw = 0 unless defined $remove_waw;
$threads = 1 unless defined $threads;

my $in  = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";

open IN, "<$in" or die "Cannot open input file $in: $!\n";
open OUT, ">$out" or die "Cannot open output file $out: $!\n";
binmode IN, ":encoding(UTF-8)";
binmode OUT, ":encoding(UTF-8)";
binmode STDERR, ":encoding(UTF-8)";


use File::Temp qw/tempfile/;
use File::Spec;
my ($ar_fh, $ar_filename) =
   tempfile("stanseg-tmp-ar.XXXX", DIR=>File::Spec->tmpdir(), UNLINK=>1);
binmode $ar_fh, ":encoding(UTF-8)";
my ($non_ar_fh, $non_ar_filename) =
   tempfile("stanseg-tmp-non-ar.XXXX", DIR=>File::Spec->tmpdir(), UNLINK=>1);
binmode $non_ar_fh, ":encoding(UTF-8)";
my ($tok_non_ar_fh, $tok_non_ar_filename) =
   tempfile("stanseg-tmp-non-ar-tok.XXXX", DIR=>File::Spec->tmpdir(), UNLINK=>1);
binmode $tok_non_ar_fh, ":encoding(UTF-8)";


### Phase 1: Split the input into two streams: Arabic and non-Arabic

my $last_arabic_id=0;
while (<IN>) {
   chomp;

   #s/[\p{Arabic}]/A/g;
   s/__ARABIC__ID/__ESCAPE_ARABIC_ID/g; # Avoid collision with ID marker

   # transform arabic numbers into English ones
   tr/٠-٩۰-۹٪٫٬/0-90-9%.,/;

   if ($xmlishify_hashtags) {
      s/&(?![a-zA-Z]+;)/&amp;/g;
      s/</&lt;/g;
      s/>/&gt;/g;
      s|#([^ #]+)|
         my $tokenized_hashtag = $1;
         $tokenized_hashtag =~ tr/_/ /;
         " <hashtag> $tokenized_hashtag </hashtag> "
      |eg;
   }

   # Extract Arabic text to feed to the Stanford Segmenter
   s/(\p{Script_Extensions: Arabic}[\p{Script_Extensions: Arabic}\s_]+)/
      #print STDERR "ARABIC $1\n";
      ++$last_arabic_id;
      print {$ar_fh} "BEGIN " if ($-[0] != 0);
      print {$ar_fh} "$1 END\n";
      " __ARABIC__ID" . $last_arabic_id . "__ "
   /eg;

   print {$non_ar_fh} $_, "\n";
}
close IN;
close $ar_fh;
close $non_ar_fh;

if ($debug) {
   system("cat $ar_filename");
   system("cat $non_ar_filename");
   system("ls -l $ar_filename $non_ar_filename");
}


### Phase 2: Tokenize the non-Arabic stream

{
   my $lang = "en";
   my $v = 0;
   my $p = 0;
   my $ss = 0;
   my $noss = 1; # We're working on OSPL: don't split sentences
   my $notok = 0;
   my $pretok = 0;
   my $paraline = 0;
   # If we are working with xml-ish tags, preserve them.
   my $xtags = $xmlishify_hashtags;

   tokenize_file($non_ar_filename, $tok_non_ar_filename, $lang, $v, $p, $ss, $noss, $notok,
                 $pretok, $paraline, $xtags) == 0
      or die "Error: stanseg.pl encountered a fatal error running tokenizer\n";
}

$debug and system("ls -l $tok_non_ar_filename");


### Phase 3: use the Stanford Segmenter on the Arabic stream

open NON_AR_IN, "<$tok_non_ar_filename" or die "Cannot opem temporarily non-Arabic stream file $tok_non_ar_filename: $!\n";
binmode NON_AR_IN, ":encoding(UTF-8)";

my $stanseg_home = $ENV{'STANFORD_SEGMENTER_HOME'};
my $stanseg_classifier = "arabic-segmenter-atb+bn+arztrain.ser.gz";
my $stanseg_cmd = "java -mx" . $threads . "g " .
      "edu.stanford.nlp.international.arabic.process.ArabicSegmenter " .
      "-loadClassifier " . $stanseg_home . "/data/" . $stanseg_classifier .
      " -prefixMarker + -suffixMarker + -domain arz -nthreads " . $threads;
open STAN_SEG_PIPE, "normalize-unicode.pl ar < $ar_filename | $stanseg_cmd |" or die "Cannot open Stanford segmenter pipe: $!\n";
binmode STAN_SEG_PIPE, ":encoding(UTF-8)";


### Phase 4: merge the processed Arabic and non-Arabic streams

while (<NON_AR_IN>) {
   s|__ARABIC__ID([0-9]+)__|
      my $contents = <STAN_SEG_PIPE>;
      die "Error: Missing line in $ar_filename for ARABIC ID$1" if (!defined $contents);
      chomp $contents;
      $contents =~ s/^BEGIN +//;
      $contents =~ s/ +END$//;
      $contents
   |eg;
   s/__ESCAPE_ARABIC_ID/__ARABIC__ID/g; # Undo collision avoidance
   s/   */ /g;
   s/ *$//;
   s/^ *//;
   s/^و\+ // if $remove_waw;
   print OUT;
}

close STAN_SEG_PIPE or die "Error closing Stanford segmenter pipe: $!\n";

