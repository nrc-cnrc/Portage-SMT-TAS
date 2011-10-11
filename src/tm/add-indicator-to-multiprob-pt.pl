#!/usr/bin/env perl
# $Id$

# @file add-indicator-column-to-pt.pl 
# @brief Add column with fixed indictor values to a multiprob phrasetable.
#
# @author George Foster
#
# COMMENTS:
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

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
printCopyright("add-indicator-to-multiprob.pl", 2007);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
$0 [-val v] [in [out]]

Add column with fixed indictor values to a multiprob phrasetable.

Options:
  -val  Use <v> as indicator [2.7183]

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
   flag        => \my $flag,
   "val=f"     => \my $val,
) or usage;

$val = 2.7183 unless defined $val;

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

open(IN, "<$in") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

while (<IN>) {
    chomp;
    my ($s,$t,$p) = split / \|\|\| /;
    my @probs = split / /, $p;
    $#probs % 2 or die "Expecting even number of columns in multiprob table\n";

    print OUT "$s ||| $t |||";
    for (my $i = 0; $i <= $#probs; ++$i) {
        print OUT " ", $probs[$i];
        if (2 * $i + 1 == $#probs) {
            print OUT " ", $val;
        }
    }
    print OUT " $val\n";
}
