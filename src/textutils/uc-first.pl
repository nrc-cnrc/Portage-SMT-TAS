#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;


# @file uc-first.pl 
# @brief 1st-letter per line uppercase mapping for iso-latin1.
# 
# @author George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

require 5.004;
use locale;
use POSIX qw(locale_h);
setlocale(LC_CTYPE, "fr_CA.iso88591");

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
printCopyright "uc-first.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


$HELP = "
uc-first.pl [in [out]]

1st-letter uppercase mapping for iso-latin1. This works regardless of how you
have your locale set up. Not really portable, though!

";

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
$in = shift || "-";
$out = shift || "-";
 
if (!open(IN, "<$in")) {die "Error: Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Error: Can't open $out for reading";}

# Enable immediate flush when piping
select(OUT); $| = 1;

while (<IN>) {print OUT ucfirst;}
