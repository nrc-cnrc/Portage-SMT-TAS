#!/usr/bin/perl
# $Id$

# @author Samuel Larkin
# @file lm_sort.pl
# @brief Sort and filter an LM optimally for maximal compression.
#
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

use strict;
use warnings;
#use IPC::Open2;

print STDERR "lm_sort.pl, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada\n";

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "

Usage: lm_sort.sh [-h(elp)] [-v(erbose)] [-d(ebug)]
       input output

  Takes an LM in ARPA format and outputs the ngram entries sorted regardless
  of their N.

  Define TMPDIR if you want to change where sort stores its temp files.

Options:
  -h(elp):      print this help message
  -v(erbose):   increment the verbosity level by 1 (may be repeated)
  -d(ebug):     print debugging information

";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
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

";
}

my $SORT_DIR = "";
#$ENV{PATH}
#$SORT_DIR="TMPDIR=$TMPDIR" if ($TMPDIR);


# Create one part per ngram size.
my $line;
my %header = ();

# Disabling support for gzip file since open seems to behave differently if one
# of the streams was open has a pipe.
#if ($in =~ /\.gz$/) {
#   open(IN, "gzip -cqfd $in |") or die "Can't open $in for reading: $!\n";
#}
#else {
#   open(IN, "<$in") or die "Can't open $in for reading: $!\n";
#}
#
#if ($in =~ /\.gz$/) {
#   open(OUT, "| gzip > $out") or die "Can't open $out for writing: $!\n";
#}
#else {
#   open(OUT, ">$out") or die "Can't open $out for writing: $!\n";
#}

open(IN, "<$in") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

local $SIG{PIPE} = sub { die "sort pipe broke" };
while (1) {
   print STDERR "." if ($debug);
   $line = <IN>;
   print OUT $line;
   last if (eof(IN));

   # Saves the number of expected entries for each ngrams.
   if ($line =~ /^ngram (\d+)=(\d+)/) {
      $header{$1} = $2;
   }

   if ($line =~ /^\\(\d)-grams:\s*$/) {
      my $N = $1;
      my $number_input_entry  = 0;
      my $number_output_entry = 0;

      $line = <IN>;

      use IPC::Open2;
      local (*READ, *WRITE);                                                                      
      my $pid = open2(\*READ, \*WRITE, "LC_ALL=C $SORT_DIR sort -t'	' -k2,2" );

      while ($line !~ /^$/) {
         print WRITE $line;
         ++$number_input_entry;
         #print STDERR "," if ($debug); # for extreme debugging ;)
         $line = <IN>;
      }
      close(WRITE);

      # Copy sorted ngram to final output.
      while (<READ>) {
         print OUT;
         ++$number_output_entry;
      }
      print OUT "\n";
      close(READ);

      if ($debug) {
         print STDERR "announced: $header{$N}\n";
         print STDERR "read:      $number_input_entry\n";
         print STDERR "writen:    $number_output_entry\n";
      }
      die "$N-gram counts doesn't match after sorting." unless ($number_input_entry == $number_output_entry);
      warn "$N-gram counts doesn't match the expected number of entries stated in the header" unless ($header{$N} == $number_output_entry);
   }
}
close(IN);
close(OUT);
