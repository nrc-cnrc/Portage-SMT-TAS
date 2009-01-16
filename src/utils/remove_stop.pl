#!/usr/bin/perl -sw

# @file remove_stop.pl 
# @brief Removes all stopwords listed in STOP_WORDS from INPUT.
#
# @author Matthew Arnold
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;

print STDERR "remove_stop.pl, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada\n";

my $HELP =
"Usage: $0 [-h] STOP_WORDS [INPUT [OUTPUT]]

Removes all stopwords listed in STOP_WORDS from INPUT.  Ignores blank lines, but if a line 
contained all stop words before, then returns back a blank line.

";

our ($h, $help);

if (defined $h || defined $help)
{
    print $HELP;
    exit;
}

my %words = ();

my $st_in = shift;
my $in = shift || "-";
my $out = shift || "-";

open(ST_IN, "<$st_in") or die "Cannot open $st_in to read stop words";
open(IN, "<$in") or die "Cannot open $in for input";
open(OUT, ">$out") or die "Cannot open $out for output";

#read in stop words
while (my $line = <ST_IN>)
{
	chomp ($line);
	$words{$line} = 1;
}

READ: while (my $line = <IN>)
{
	chomp($line);
	my @list = split(/ /, $line);
	if ( ! @list ) {
		print OUT "\n";
		next READ;
	}
	my $w = shift(@list);
	while ( exists $words{$w} ) {
		$w = shift(@list) || "";
		if ($w eq "") {
			print OUT "\n";
			next READ;
		}
	}
	print OUT $w;
	foreach my $wd (@list) {
		#if not in the stop word list, print it
		print OUT " " . $wd if not exists $words{$wd};
	}
	print OUT "\n";
} # while

