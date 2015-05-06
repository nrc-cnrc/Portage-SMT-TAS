#!/usr/bin/perl -s

# @file randomly-choose-n-args.pl 
# @brief Choose n out of the given m args and write them to stdout.
# 
# @author George Foster
# 
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
printCopyright "randomly-choose-n-args.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP = "
randomly-choose-n-args.pl -with-replacement n arg1 arg2 ... argm

Choose n out of the given m args and write them to stdout.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}

our $with_replacement = 0 unless defined $with_replacement;

my $n = shift or die "Error: n must be specified.\n";

if (!$with_replacement && $n > $#ARGV+1) {
   die "Error: n cannot be greater than number of args.\n";
}

foreach my $i (1..$n) {

   my $r = int rand($#ARGV+1);
   print $ARGV[$r], "\n";
   if (!$with_replacement) {splice(@ARGV, $r, 1);}

}
