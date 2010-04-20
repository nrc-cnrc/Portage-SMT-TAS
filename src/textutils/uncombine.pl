#!/usr/bin/perl -s

# @file uncombine.pl 
# @brief Split by stripping.
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
printCopyright "uncombine.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


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

