#!/usr/bin/perl -s

# strip-markup.pl
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

print STDERR "strip-markup.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

my $HELP = "
strip-markup.pl [in [out]]

Strip standard markup from a text file. Assumes tags are plain alphanums.

Options:

-v Write interesting messages to stderr.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
my $in = shift || "-";
my $out = shift || "-";
 
open(IN, "<$in") or die "Can't open $in for reading\n";
open(OUT, ">$out") or die "Can't open $out for writing\n";

while (<IN>) {

    s/<([a-zA-Z0-9]+)[^>]*>([^<]*)<\/\1>/$2/go;
    s/[ ]+/ /go;
    print OUT;
}
