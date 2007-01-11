#!/usr/bin/perl -s

# strip-parallel-blank-lines.pl
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

print STDERR "strip-parallel-blank-lines.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

my $HELP = "
strip-parallel-blank-lines.pl file1 file2

Strip parallel blank lines from two line-aligned files. Write output to <file1>.no-blanks

Options:

-v Write interesting messages to stderr.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
my $in1 = shift or die $HELP;
my $in2 = shift or die $HELP;
 
open(IN1, "<$in1") or die "Can't open $in1 for reading\n";
open(IN2, "<$in2") or die "Can't open $in2 for reading\n";
open(OUT1, ">$in1.no-blanks") or die "Can't open $in1.no-blanks for writing\n";
open(OUT2, ">$in2.no-blanks") or die "Can't open $in2.no-blanks for writing\n";

my ($line1, $line2);
while ($line1 = <IN1>) {
    if (!($line2 = <IN2>)) {die "file $in2 is too short!\n";}
    if ($line1 !~ /^\s*$/o || $line2 !~ /^\s*$/o) {
        print OUT1 $line1;
        print OUT2 $line2;
    }
}

if (<IN2>) {die "file $in1 is too short!\n";}
