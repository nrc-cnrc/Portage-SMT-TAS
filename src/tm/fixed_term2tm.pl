#!/usr/bin/env perl

# @file fixed_term2tm.pl
# @brief Create a phrase table to mark fixed terms.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

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
use ULexiTools;

printCopyright "fixed_term2tm.pl", 2014;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] IN [OUT]

  Given a list of fixed terms in a two-column tab-separated file, convert it
  to a translation table.

Options:

  -source=language  Source language
  -target=language  Target language
  -source_column=index  1-based source column index [1]

  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -d(ebug)      print debugging information
";
   exit @_ ? 1 : 0;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
my $sourceColumn = 1;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,

   "source=s"  => \my $sourceLanguage,
   "target=s"  => \my $targetLanguage,
   "source_column=i" => \$sourceColumn,

   flag        => \my $flag,
   "opt_with_string_arg=s"  => \my $opt_with_string_arg_value,
   "opt_with_integer_arg=i" => \my $opt_with_integer_arg_value,
   "opt_with_float_arg=f"   => \my $opt_with_float_arg_value,
) or usage "Error: Invalid option(s).";

my $in = shift or die "Error: You must provide an input file";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";
die "Error: You must provide a source language." unless(defined($sourceLanguage));
die "Error: You must provide a target language." unless(defined($targetLanguage));

die "Error: Invalid source column index ($sourceColumn)" unless ($sourceColumn == 1 or $sourceColumn == 2);

# Find the 1-based index of the target language based on the source language column.
my $targetColumn = $sourceColumn % 2 + 1;
die "Error: targetColumn cannot be the same as sourceColumn" unless($targetColumn != $sourceColumn);
die "Error: Something wrong with the column indices." unless ($targetColumn + $sourceColumn == 3);

if ( $debug ) {
   no warnings;
   print STDERR "
   in          = $in
   out         = $out
   verbose     = $verbose
   debug       = $debug
   flag        = $flag
   sourceColumn   = $sourceColumn
   targetColumn   = $targetColumn
   sourceLanguage = $sourceLanguage
   targetLanguage = $targetLanguage

";
}

# This list should actually contain utokenize.pl's valid language codes.
# Ref for ISO 639-1/2 codes: https://www.loc.gov/standards/iso639-2/php/code_list.php
my %languageMap = (
   fra=>'fr', # ISO 639-2 (T) code for French
   fre=>'fr', # ISO 639-2 (B) code for French
   fr=>'fr',  # ISO 639-1 code for French
   eng=>'en', # ISO 639-2 code for English
   en=>'en',  # ISO 639-1 code for English
   dan=>'da', # ISO 639-2 code for Danish
   da=>'da',  # ISO 639-1 code for Danish
   spa=>'es', # ISO 639-2 code for Spanish
   es=>'es',  # ISO 639-1 code for Spanish
);

$sourceLanguage = lc($sourceLanguage);
$targetLanguage = lc($targetLanguage);

if (defined($languageMap{$sourceLanguage})) {
   $sourceLanguage = $languageMap{$sourceLanguage};
}
else {
   die "Error: Unsupported source language ($sourceLanguage)";
}

if (defined($languageMap{$targetLanguage})) {
   $targetLanguage = $languageMap{$targetLanguage};
}
else {
   die "Error: Unsupported target language ($targetLanguage)";
}


my $command = "set -o pipefail; zcat --force $in | cut --only-delimited --fields=%d | utokenize.pl -noss -lang=%s | utf8_casemap -cl |";
# Note, don't canoe-escapes.pl the input, the source side in the phrase table mustn't be escaped.
zopen(*SRC, sprintf($command, $sourceColumn, $sourceLanguage)) or die "Error: Can't open $in for reading: $!\n";
zopen(*TGT, sprintf($command, $targetColumn, $targetLanguage)) or die "Error: Can't open $in for reading: $!\n";
zopen(*OUT, ">$out") or die "Error: Can't open $out for writing: $!\n";

binmode( SRC, ":encoding(UTF-8)" );
binmode( TGT, ":encoding(UTF-8)" );
binmode( OUT, ":encoding(UTF-8)" );

sub normalize {
   my $data = shift;
   chomp($data);
   $data =~ s/ +$//;
   $data =~ s/([\\<>])/\\$1/g;  # canoe-escapes.pl -add
   return $data;
}

my $sourceTerm = undef;
my $targetTerm = undef;
# First line should indicate the language id of each column.
if (defined($sourceTerm = <SRC>) and defined($targetTerm = <TGT>)) {
   $sourceTerm = normalize($sourceTerm);
   $targetTerm = normalize($targetTerm);
   # Should these checks be error fatal if the languages don't match?
   die "Error: Language mismatch for source language(file: $sourceTerm, cli: $sourceLanguage)."
      unless ($sourceLanguage eq $languageMap{$sourceTerm});
   die "Error: Language mismatch for target language(file: $targetTerm, cli: $targetLanguage)."
      unless ($targetLanguage eq $languageMap{$targetTerm});
}
else {
   die "Error: Corrupted file, missing the language header.";
}

my $counter = 0;
while (defined($sourceTerm = <SRC>) and defined($targetTerm = <TGT>)) {
   $sourceTerm = normalize($sourceTerm);
   $targetTerm = normalize($targetTerm);
   next if ($sourceTerm eq '' and $targetTerm eq '');
   if ($sourceTerm eq '' or $targetTerm eq '') {
      warn "Warning: Invalid terms ('$sourceTerm'\t'$targetTerm').";
      next;
   }
   print OUT "$sourceTerm ||| <FT target=\"$targetTerm\">$sourceTerm</FT> ||| 1 1\n";
   ++$counter;
}

close(SRC) or die "Error: Not all source terms were read.";
close(TGT) or die "Error: Not all target terms were read.";
close(OUT);

print STDERR $counter;
