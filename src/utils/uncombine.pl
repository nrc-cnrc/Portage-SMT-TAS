#!/usr/bin/perl -s

# uncombine.pl
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

print STDERR "uncombine.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

my $HELP = "
uncombine.pl n infile

Options:

Split <infile> into files of size <n> by writing the 1st line of <infile> to
<infile>.1, the 2nd to <infile>.2 ... <infile>.<s>, where s is the number of
lines in <infile> / n. This can be used to convert from a rescore_train format
reference file to a bleumain format reference file. (Use combine.pl to go in
the other direction.)

<n> / the number of lines in infile must be an integer.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
my $n = shift || die "Error: missing n parameter.\n$HELP";
my $in = shift || die "Error: missing infile parameter\n$HELP";

open(IN, "<$in") or die "Can't read from $in\n";
my @lines = <IN>;
close IN;

if (($#lines+1) % $n != 0) {
    die "Error: can't split $in evenly into files of size $n\n";
}

my $s = ($#lines+1) / $n;


foreach my $i (1 .. $s) {
    open(OUT, ">$in.$i") or die "Can't write to $in.$i\n";
    for (my $j = $i-1; $j <= $#lines; $j += $s) {
	print OUT $lines[$j];
    }
    close OUT;
}

