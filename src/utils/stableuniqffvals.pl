#!/usr/bin/perl -sw

# stableuniqffvals.pl
# 
# PROGRAMMER: Matthew Arnold
#		based on similar programs by Aaron Tikuisis
# 
# COMMENTS:
# 
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada
#
# Takes a single file from canoe's ffvals output and creates a unique list from that file (unique phrase, not unique ffvals)

use strict;

print STDERR "stableuniqffvals.pl, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada\n";

my $HELP =
"Usage: $0 [INPUT [OUTPUT]]

Outputs unique lines from INPUT (or standard input) to OUTPUT (or standard output).  INPUT
is not required to be sorted, and lines are not reordered (as opposed to using sort and
uniq to find unique lines).

";

our ($h, $help);

if (defined $h || defined $help)
{
    print $HELP;
    exit;
}
#converts a line of ffvals text to a standard string
sub extract_string {
    my $curr = '';
	my $line = $_[0];
	my $ind = index($line, "\"");
	while ( $ind != -1 ) {
		my $nextind = index($line, "\"", $ind + 1);
		$curr = $curr . substr($line, $ind + 1, $nextind - $ind - 1);
		$ind = index($line, "\"", $nextind + 1);
	}
	return $curr;
}

my %existing = ();

my $in = shift || "-";
my $out = shift || "-";

open(IN, "<$in") or die "Cannot open $in for input";
open(OUT, ">$out") or die "Cannot open $out for output";

while (my $line = <IN>)
{
    print OUT $line if not exists $existing{extract_string($line)};
    $existing{extract_string($line)} = 1;
} # while
