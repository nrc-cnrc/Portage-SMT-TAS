#!/usr/bin/env perl

# @file lm_sort.pl
# @brief Sort and filter an LM optimally for maximal compression.
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
#use IPC::Open2;

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
printCopyright("lm_sort.pl", 2008);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "

Usage: lm_sort.pl [-h(elp)] [-v(erbose)] [-d(ebug)]
       input output

  Takes an LM in ARPA format and outputs the ngram entries sorted regardless
  of their N.

  Define TMPDIR if you want to change where sort stores its temp files.

Options:
  -h(elp):      print this help message
  -v(erbose):   increment the verbosity level by 1 (may be repeated)
  -d(ebug):     print debugging information

";
   exit @_ ? 1 : 0;
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
) or usage "Error: Invalid option(s).";

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";


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

# TODO: replace all this with zin/zout in utils.pm
if ($in =~ /\|\s*$/) {
   open(IN, "$in") or die "Error: Can't open $in for reading: $!\n";
}
elsif ($in =~ /\.gz$/) {
   open(IN, "gzip -cqfd $in |") or die "Error: Can't open $in for reading: $!\n";
}
else {
   open(IN, "<$in") or die "Error: Can't open $in for reading: $!\n";
}

if ($out =~ /^\s*\|/) {
   open(OUT, "$out") or die "Error: Can't open $out for writing: $!\n";
}
elsif ($out =~ /\.gz$/) {
   open(OUT, "| gzip > $out") or die "Error: Can't open $out for writing: $!\n";
}
else {
   open(OUT, ">$out") or die "Error: Can't open $out for writing: $!\n";
}

#open(IN, "<$in") or die "Error: Can't open $in for reading: $!\n";
#open(OUT, ">$out") or die "Error: Can't open $out for writing: $!\n";

local $SIG{PIPE} = sub { die "Error: sort pipe broke" };
while (1) {
   print STDERR "." if ($debug);
   $line = <IN>;
   last if (eof(IN));
   print OUT $line;

   # Saves the number of expected entries for each ngrams.
   if ($line =~ /^ngram (\d+)=\s*(\d+)/) {
      $header{$1} = $2;
   }

   if ($line =~ /^\\(\d)-grams:\s*$/) {
      my $N = $1;
      my $number_input_entry  = 0;
      my $number_output_entry = 0;

      warn "Warning: Can't find ${N}gram in the header" unless (defined($header{$N}));

      $line = <IN>;

      use IPC::Open2;
      local (*READ, *WRITE);                                                                      
      my $pid = open2(\*READ, \*WRITE, "LC_ALL=C $SORT_DIR sort -t'	' -k2,2" );

      while ($line !~ /^(\\end\\)*$/) {
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
         print STDERR "expected $N-grams: $header{$N}\n";
         print STDERR "read: $number_input_entry\n";
         print STDERR "writen: $number_output_entry\n";
      }
      die "Error: $N-gram counts doesn't match after sorting." unless ($number_input_entry == $number_output_entry);
      warn "Warning: $N-gram counts doesn't match the expected number of entries stated in the header" unless ($header{$N} == $number_output_entry);
   }
}
print OUT "\\end\\\n";
close(IN);
close(OUT);
