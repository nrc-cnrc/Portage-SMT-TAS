#!/usr/bin/perl -s

# maxphrase.pl Finds the maximum phrase lengths in a phrase table
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "maxphrase.pl, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada\n";

our ($h, $help, $stats, $filtdiff);

my $HELP = "
Usage: maxphrase.pl [-stats | -filtdiff=d] [phrasetable]
Scans the given phrase translation table and finds the maximum phrase lengths.
The maximum lengths of the left and right phrases are outputted, separated by a
tab.

Options:
  -stats Print stats on number of phrase pairs versus their lengths.
  -filtdiff Write the input table to stdout, but filter out any phrase pairs 
      whose lengths differ by more than d.

";

if (defined $h || defined $help)
{
    print $HELP;
    exit;
} # if

die "Error: can't use -stats and -filtdiff at the same time\n"
    if (defined $stats && defined $filtdiff);

my $M1 = 0;
my $M2 = 0;
my %counts = ();

my $in = shift || "-";

open(IN, "<$in") or die "Cannot open $in for input";

my $lineNum = 0;
while (my $line = <IN>)
{
    $lineNum++;
    if ($line =~ /^(.*)\|\|\| (.*)\|\|\|.*$/) 
    {
	my $count1 = ($1 =~ tr/ //);
	$M1 = $count1 if ($count1 > $M1);
	my $count2 = ($2 =~ tr/ //);
	$M2 = $count2 if ($count2 > $M2);
	
	if (defined $filtdiff) 
	{
	    if (abs($count1 - $count2) <= $filtdiff) 
	    {
		print $line;
	    }
	} 
	elsif ($stats) 
	{
	    ++$counts{"$count1,$count2"};
	}
    } 
    else 
    {
	print "Warning: invalid phrase table format at line $lineNum\n" unless $line == "";
    }
}

if ($stats) {
    foreach my $key (keys %counts) {
	print $key, "  ", $counts{$key}, "\n";
    }
} elsif (!defined $filtdiff) {
    print "$M1\t$M2\n";
}
