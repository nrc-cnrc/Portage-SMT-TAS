#!/usr/bin/env perl

# @file filter-latin.pl 
# @brief Filter out invalid characters from an iso-latin1 encoded file.
#
# @author George Foster
#
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
printCopyright "filter-latin.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
    local $, = "\n";
    print STDERR @_, "";
    $0 =~ s#.*/##;
    print STDERR "
Usage: $0 [in [out]]

Filter out invalid characters from an iso-latin1 encoded file.

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
) or usage "Error: Invalid option(s).";

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";

open(IN, "<$in") or die "Error: Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Error: Can't open $out for writing: $!\n";

while (<IN>) {
    s/[\000-\010\013-\014\016-\037\177-\237]//go;
    print OUT;
}
