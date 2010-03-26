#!/usr/bin/perl
# $Id$

# @file tt2ptbest.pl
# @brief ttable to phrate table filtered on source.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

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
printCopyright("tt2ptbest.pl", 2008);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] ttable [jpt]

Generate a joint phrase table from an IBM/HMM ttable, retaining a given number
of best (most probable) translations for each source word in the ttable. Write
output to <jpt> or stdout.

Options:

-n   The number of best translations to keep. [1]
-r   Reverse the order of src and tgt columns in the output [don't]
-s s Set output probabilities to int(s * p) + 1, where p is the input
     (IBM/HMM) probability. [0]
";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
my $max_translation = 1;
my $scale = 0;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
   r           => \my $reverse,
   "n=i"       => \$max_translation,
   "s=f"       => \$scale,
) or usage;

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

if ( $debug ) {
   no warnings;
   print STDERR "
   in          = $in
   out         = $out
   verbose     = $verbose
   debug       = $debug
   reverse     = $reverse
   n           = $max_translation
   s           = $scale

";
}

# This function depends on $max_translation, $reverse & $scale.
sub print_record ($\%) {
   my ($previous_source, $data) = @_;
   my $print_count = 0;
   foreach my $key (sort {$data->{$b} <=> $data->{$a}} keys %$data) {
      #print STDERR "\t<D> KEY: $key $read_count $max_translation\n" if ($debug);
      if ($print_count < $max_translation) {
         ++$print_count;
         my $count = ($scale * $data->{$key}) + 1;
         if (defined($reverse)) {
            printf(OUT "%s ||| %s ||| %d\n", $key, $previous_source, $count);
         }
         else {
            printf(OUT "%s ||| %s ||| %d\n", $previous_source, $key, $count);
         }
      }
   }
}

open(IN, "gzip -cqfd $in |") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

my $line = "";
my $number_of_different_source = 0;
my $number_of_entries = 0;
my $previous_source = "";
my %data = ();
while ($line = <IN>) {
   chomp($line);
   my ($source, $target, $prob) = split(/\s+/, $line, 3);
   print STDERR "\t<D> Read: $source $target $prob\n" if ($debug);

   # Is this a new source?
   if ($source ne $previous_source) {
      printf(STDERR "\t<D> New source found $previous_source(%d) => $source\n", scalar keys(%data))  if ($debug);

      print_record($previous_source, %data);

      %data = ();
      $previous_source = $source;
      ++$number_of_different_source;
   }

   # Keep a copy
   $data{$target} = $prob;

   ++$number_of_entries;
}

# And print the last source word
print_record($previous_source, %data);

print STDERR "\t<D> There were $number_of_different_source/$number_of_entries different sources!\n" if ($debug);
