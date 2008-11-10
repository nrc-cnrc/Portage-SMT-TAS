#!/usr/bin/perl
# $Id$

# @file filter-latin.pl 
# @brief Filter out invalid characters from an iso-latin1 encoded file.
#
# @author George Foster
#
# COMMENTS:
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "filter-latin.pl, NRC-CNRC, (c) 2005 - 2008, Her Majesty in Right of Canada\n";

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
) or usage;

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

open(IN, "<$in") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

while (<IN>) {
    s/[\000-\010\013-\014\016-\037\177-\237]//go;
    print OUT;
}
