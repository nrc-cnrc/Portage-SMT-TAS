#!/usr/bin/perl -sw
# @file
# @brief
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

use strict;
use warnings;

our($help, $h, $H, $verbose, $debug, $input, $output, $path, $stats);

my $model_name = shift or die "Missing argument: model";
my $data_dir = shift || ".";
$input  = "${data_dir}/Q.txt" unless defined $input;
$output = "${data_dir}/pr.ce" unless defined $output;

open(my $in, "< $input") or die "Can't open input file $input";
open(my $out, "> $output") or die "Can't open input file $output";
while (<$in>) {
   print $out "$.\n";
}
close($in);
close($out);

