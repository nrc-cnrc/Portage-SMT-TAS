#!/usr/bin/perl
# $Id$

# filter-latin.pl Program template
#
# PROGRAMMER: George Foster
#
# COMMENTS:
#
# George Foster
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use strict;
use warnings;

print STDERR "filter-latin.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

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
