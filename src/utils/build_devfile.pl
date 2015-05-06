#!/usr/bin/perl -sw

# @file build_devfile.pl
# @brief Build randomly a dev file based on a certain number of lines 
#
# @author Fatiha Sadat
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

#use strict;

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
printCopyright "build_devfile.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP = "
build_devfile.pl num [in [out]]

Build randomly a dev file based on a specified number (num) of lines

Options:

Do not forget to specify the number of lines you want to print


";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}

my $p = shift || "-";
my $in = shift || "-";   
my $out = shift || "-";

open(IN, $in) || die "Error: Can't open $in for reading";
open(OUT, ">$out") || die "Error: Can't open $out for writing";
my $i=0;

while(<IN>){
my $line=$_;
$i++;
$tab{$i}=$line;
}

my $x=0;
my $q=$p*2;
foreach $k (keys %tab){
if (($k=$x+1) && ($k < $q)){ print OUT "$tab{$k}"; $x=$x+2; } 
}


close(IN);
close(OUT);
