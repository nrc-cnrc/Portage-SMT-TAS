#!/usr/bin/perl -sw

#  Build randomly a dev file based on a certain number of lines 
#
# PROGRAMMER: Fatiha Sadat
# 
# COMMENTS: 
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

#use strict;
#use LexiTools;

print STDERR "build_devfile.pl, Copyright (c) 2005 - 2006, Sa Majeste la Reine du Chef du Canada / Her Majesty in Right of Canada\n";

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

open(IN, $in) || die "Can't open $in for reading";
open(OUT, ">$out") || die "Can't open $out for writing";
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



