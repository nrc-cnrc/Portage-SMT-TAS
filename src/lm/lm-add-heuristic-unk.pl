#!/usr/bin/env perl

# @file lm-add-heuristic-unk.pl 
# @brief Take an LM in ARPA format an insert a heuristic p(<unk>) value.
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
Usage: $0 [options] [IN [OUT]]

  Take an LM in ARPA format an insert a heuristic log p(<unk>) value.

  This value is set to be delta lower than the lowest unigram log prob found
  in the model, which is typically a singleton log prob.

  If the model already has log p(<unk>), it is overridden.

  If the model contains -99 or -inf, (e.g., for <s>), those lines are ignored
  in determining the minimum log prob found.

Options:

  -delta D      delta log prob to apply [0.15]
  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -debug        print debugging information
";
   exit 1;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
my $delta = 0.15;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
   "delta=f"   => \$delta,
) or usage "Error: Invalid option(s).";

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";

zopen(*IN, "<$in") or die "Error: Can't open $in for reading: $!\n";
zopen(*OUT, ">$out") or die "Error: Can't open $out for writing: $!\n";

my $unigram_count;
my $unigram_index;
my @header;
while (<IN>) {
   push @header, $_;
   last if /^\\1-grams:/;
   if (/\s*ngram\s*(\d+)\s*=\s*(\d+)/) {
      $unigram_count = $2 if ($1 == 1);
      $unigram_index = $#header;
   }
   last if ($. > 1000) # safety for bad input files
}
defined $_ and /\\1-grams:/ or die "Error: input file $in does not seem to be an ARPA LM file.\n";
defined $unigram_index or die "Error: Did not find unigram info in data section.\n";

$verbose and print STDERR "Reading lm $in with $unigram_count unigrams.\n";

my @unigram_lines;
my @inf_lines;
my $unk_line;
my $bos_line;
my $min_prob = 0;
my $min_prob_count = 0;
my $second_min = 0;
my $second_min_count = 0;
while (<IN>) {
   push @unigram_lines, $_;
   last if /^\\2-grams:/ or /^\\end\\/;
   chomp;
   my @tokens = split /\t/;
   if (@tokens > 1) {
      if ($tokens[1] eq "<unk>") {
         $verbose and print STDERR "Removing existing <unk> entry:    $_\n";
         $unk_line = $_;
         pop @unigram_lines;
      } elsif ($tokens[1] eq "<s>") {
         $verbose and print STDERR "Ignoring <s> prob:                $_\n";
         $bos_line = $_;
      } elsif ($tokens[0] eq "-99" or $tokens[0] eq "-inf") {
         # ignore -99 / -inf in min logprob calculation
         $verbose and print STDERR "Ignoring -99/-inf prob:           $_\n";
         push @inf_lines, $_;
      } else {
         # Normal line - do min calculations
         if ($tokens[0] < $min_prob) {
            $second_min = $min_prob;
            $second_min_count = $min_prob_count;
            $min_prob = $tokens[0];
            $min_prob_count = 1;
         } elsif ($tokens[0] == $min_prob) {
            $min_prob_count += 1;
         } elsif ($tokens[0] < $second_min) {
            $second_min = $tokens[0];
            $second_min_count = 1;
         } elsif ($tokens[0] == $second_min) {
            $second_min_count += 1;
         }
      }
   }
}

my $new_unk_prob = $min_prob - $delta;

if ($verbose) {
   print STDERR "Second min      $second_min count $second_min_count\n";
   print STDERR "Min logprob     $min_prob count $min_prob_count\n";
   print STDERR "New unk logprob $new_unk_prob\n"
}

if (defined $unk_line) {
   print OUT @header;
} else {
   for (@header) {
      if (/\s*ngram\s*1\s*=\s*(\d+)/) {
         print OUT "ngram 1=", $1+1, "\n";
      } else {
         print OUT;
      }
   }
}

print OUT "$new_unk_prob\t<unk>\n";
print OUT @unigram_lines;

while (<IN>) {
   print OUT;
}

close(IN);
close(OUT);

