#!/usr/bin/perl -s

# @file canoe-escapes.pl
# @brief Escapes input text for canoe (\ < >).
# 
# @author Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

use strict;
use warnings;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   print STDERR "
Usage: canoe-escapes.pl [IN [OUT]]

  Canoe requires that \\, < and > be escaped using \\ in its input.  This is
  normally done by a parser for the source language for things like dates and
  numbers.  When no parser is used, this script can be used instead.

Options:

  -a(dd)     Add canoe escapes [default]
  -r(emove)  Remove existing canoe escapes, i.e., undo what -add did.
  -h(elp)    Print this help message

";
   exit 1;
}

our ($help, $h, $add, $a, $remove, $r);

usage if ($help || $h);

my $in = shift || "-";
my $out = shift || "-";
0 == @ARGV or usage "Superfluous parameter(s): @ARGV";
 
open(IN, "<$in") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

if ( $remove || $r ) {
   while (<IN>) {
      s/\\([\\<>])/$1/g;
      print OUT;
   }
} else {
   while (<IN>) {
      s/([\\<>])/\\$1/g;
      print OUT;
   }
}
