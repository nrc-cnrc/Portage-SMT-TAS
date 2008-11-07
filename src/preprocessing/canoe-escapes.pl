#!/usr/bin/perl -s

# add-standard-escapes.pl
# 
# PROGRAMMER: Eric Joanis
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

use strict;
use warnings;

my $HELP = "
canoe-escapes.pl [IN [IN2 [IN3 ...]]]

  Canoe requires that \\, < and > be escaped using \\ in its input.  This is
  normally done by a parser for the source language for things like dates and
  numbers.  When no parser is used, this script can be used instead.

Options:

  -a(dd)     Add canoe escapes [default]
  -r(emove)  Remove existing canoe escapes, i.e., undo what -add did.
  -h(elp)    Print this help message

";

our ($help, $h, $add, $a, $remove, $r);

if ($help || $h) {
    print $HELP;
    exit 1;
}
 
if ( $remove || $r ) {
   while (<>) {
      s/\\([\\<>])/$1/g;
      print;
   }
} else {
   while (<>) {
      s/([\\<>])/\\$1/g;
      print;
   }
}
