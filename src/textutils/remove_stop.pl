#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;

# @file remove_stop.pl 
# @brief Remove all stopwords listed in STOP_WORDS from INPUT.
#
# @author Matthew Arnold
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;

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
printCopyright "remove_stop.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP =
"Usage: remove_stop.pl [-h] STOP_WORDS [INPUT [OUTPUT]]

Remove all stopwords listed in STOP_WORDS from INPUT. Ignores blank lines,
but if a line contained all stop words before, then returns back a blank line.

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

open(ST_IN, "<$st_in") or die "Error: Cannot open $st_in to read stop words";
open(IN, "<$in") or die "Error: Cannot open $in for input";
open(OUT, ">$out") or die "Error: Cannot open $out for output";

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

