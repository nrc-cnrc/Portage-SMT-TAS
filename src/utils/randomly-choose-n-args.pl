#!/usr/bin/perl -s

# randomly-choose-n-args.pl
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

print STDERR "randomly-choose-n-args.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

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
 
my $n = shift or die "n must be specified\n";

if (!$with_replacement && $n > $#ARGV+1) {
   die "n cannot be greater than number of args\n";
}

foreach my $i (1..$n) {

   my $r = int rand($#ARGV+1);
   print $ARGV[$r], "\n";
   if (!$with_replacement) {splice(@ARGV, $r, 1);}

}
