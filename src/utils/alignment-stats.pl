#!/usr/bin/perl
# $Id$

# @file alignment-stats.pl 
# @brief produce some statistics that might suggest the quality of alignment
# sentence aligned files.
#
# @author Eric Joanis
#
# COMMENTS:
#
# Eric Joanis
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

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
printCopyright "alignment-stats.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
    local $, = "\n";
    print STDERR @_, "";
    $0 =~ s#.*/##;
    print STDERR "
Usage: $0 [-h(elp)] [-v(erbose)] [-r(atio) <r>] file1 file2

  Produce statistics comparing the number of words in corresponding lines of
  file1 and file2.

Options:

  -r(atio) <r>  print lines where one is more than <r> times longer than the
                other
  -0            omit any lines containing 0 words from output
  -h(elp):      print this help message
  -v(erbose):   increment the verbosity level by 1 (may be repeated)
  -d(ebug):     print debugging information

";
    exit 1;
}

use Getopt::Long;
my $verbose = 1;
GetOptions(
    help        => sub { usage },
    verbose     => sub { ++$verbose },
    quiet       => sub { $verbose = 0 },
    debug       => \my $debug,
    "ratio=f"   => \my $ratio,
    0           => \my $nozeros,
) or usage;

my $file1 = shift || "-";
my $file2 = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

if ( $debug ) {
    no warnings;
    print STDERR "
    file1       = $file1
    file2       = $file2
    verbose     = $verbose
    debug       = $debug

";
}

open(FILE1, "$file1") or die "Can't open $file1 for reading: $!\n";
open(FILE2, "$file2") or die "Can't open $file2 for writing: $!\n";

# count the number of tokens in corresponding lines of each input file, and
# then count how often matching counts occur.
my %counts;
my $line = 0;
while (<FILE1>) {
    $line++;
    my $line1 = $_;
    my $line2 = <FILE2>;
    if ( ! defined $line2 ) {
        print STDERR "$file2 shorter than $file1: unexpected EOF at line $line\n";
        last;
    }
    
    my $line1_count = scalar @{[ split /\s+/, $line1 ]};
    my $line2_count = scalar @{[ split /\s+/, $line2 ]};

    $counts{"$line1_count $line2_count"}++;

    if ( defined $ratio && (! defined $nozeros || $line1_count && $line2_count)) {
        if ( $line1_count * $ratio < $line2_count or
             $line2_count * $ratio < $line1_count ) {
            print "$file1:$line($line1_count words):$line1";
            print "$file2:$line($line2_count words):$line2";
            print "\n";
        }
    }
}
if (defined <FILE2>) {
    print STDERR "$file1 shorter than $file2: unexpected EOF at line $line\n";
}

# Print the results, sorted first by the number of tokens of lines in the first
# file, then in the second file, using a Schwarzian transform.  (See Hall with
# Schwartz 1998, Effective Perl Programming, if you don't know what this means
# or don't understand this code, or ask Eric Joanis)
foreach (
    sort { $a->[1] <=> $b->[1] || $a->[2] <=> $b->[2] } # sort on 12, 15
    map { [$_, split(" ", $_)] }        # maps "12 15" to ["12 15", 12, 15]
    keys %counts                        # extract the count pair, e.g., "12 15"
)
{
    # Here, $_ points to triples ["12 15", 12, 15], sorted.
    if (! defined $nozeros || $_->[1] && $_->[2]) {
        printf "%-8d %-3d %-3d\n", $counts{$_->[0]}, $_->[1], $_->[2];
    }
}
