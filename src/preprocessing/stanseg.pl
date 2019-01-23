#!/usr/bin/env perl

# @file prog.pl 
# @brief Briefly describe your program here.
#
# @author Write your name here
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2019, Sa Majeste la Reine du Chef du Canada /
# Copyright 2019, Her Majesty in Right of Canada

use utf8;
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

  -m    xmlishify hashtags
  -w    remove waw (w+) prefix at the beginning of the sentence

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
) or usage "Error: Invalid option(s).";

my $in  = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";

open IN, "<$in" or die "Cannot open input file $in: $!\n";
open OUT, ">$out" or die "Cannot open output file $out: $!\n";
binmode IN, ":encoding(UTF-8)";
binmode OUT, ":encoding(UTF-8)";
binmode STDERR, ":encoding(UTF-8)";

use File::Temp qw/tempfile/;
my ($ar_fh, $ar_filename) = tempfile(UNLINK=>1);
my ($non_ar_fh, $non_ar_filename) = tempfile(UNLINK=>1);
binmode $ar_fh, ":encoding(UTF-8)";
binmode $non_ar_fh, ":encoding(UTF-8)";

my $last_arabic_id=0;
while (<IN>) {
   chomp;

   #s/[\p{Arabic}]/A/g;
   s/__ARABIC__ID/__ESCAPE_ARABIC_ID/g; # Avoid collision with ID marker

   # transform arabic numbers into English ones
   tr/٠-٩٪٫٬/0-9%.,/;

   # Extract Arabic text to feed to the Stanford Segmenter
   s/(\p{Arabic}[\p{Arabic}\s_]+)/
      #print STDERR "ARABIC $1\n";
      ++$last_arabic_id;
      print {$ar_fh} $1, "\n";
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

open NON_AR_IN, "<$non_ar_filename" or die "Cannot opem temporarily non-Arabic stream file $non_ar_filename: $!\n";
binmode NON_AR_IN, ":encoding(UTF-8)";

my $stanseg_home = $ENV{'STANFORD_SEGMENTER_HOME'};
my $stanseg_classifier = "arabic-segmenter-atb+bn+arztrain.ser.gz";
my $stanseg_cmd = "java -mx1g " .
      "edu.stanford.nlp.international.arabic.process.ArabicSegmenter " .
      "-loadClassifier " . $stanseg_home . "/data/" . $stanseg_classifier .
      " -prefixMarker + -suffixMarker + -domain arz -nthreads 1";
open STAN_SEG_PIPE, "$stanseg_cmd < $ar_filename |" or die "Cannot open Stanford segmenter pipe: $!\n";
binmode STAN_SEG_PIPE, ":encoding(UTF-8)";

while (<NON_AR_IN>) {
   s/__ARABIC__ID([0-9]+)__/
      my $contents = <STAN_SEG_PIPE>;
      die "Error: Missing line in $ar_filename for ARABIC ID$1" if (!defined $contents);
      chomp $contents;
      $contents
   /eg;
   s/   */ /g;
   s/ *$//;
   s/^ *//;
   s/'^و\+ '// if $remove_waw;
   print OUT;
}

close STAN_SEG_PIPE or die "Error closing Stanford segmenter pipe: $!\n";

