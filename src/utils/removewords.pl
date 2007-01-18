#!/usr/bin/perl -sw

# removewords.pl Filters out words from a file
# 
# PROGRAMMER: Aaron Tikuisis
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

print STDERR "removewords.pl, Copyright (c) 2005 - 2006, Sa Majeste la Reine du Chef du Canada / Her Majesty in Right of Canada\n";

my $HELP =
"Usage: removewords.pl WORDLISTFILE [INFILE [OUTFILE]]

Filters out all of the words in WORDLISTFILE, using INFILE as input and OUTFILE for
output.

Options:
WORDLISTFILE
	The file containing the list of words to remove.  Words must be seperated by
	spaces and/or newlines.
INFILE
	The file to get input from.  [-]
OUTFILE
	The file to get output from.  [-]
";

our ($help, $h);
if (defined $h or defined $help)
{
    print $HELP;
    exit 1;
} # if

my $wordfile = shift || die "No word list given.  Use -h for help.";
my $in = shift || "-";
my $out = shift || "-";
open(WIN, "<$wordfile") || die "Cannot open $wordfile for input";
open(IN, "<$in") || die "Cannot open $in for input";
open(OUT, ">$out") || die "Cannot open $out for output";

# Read word list and store as hash
my %rwords = ();
while (my $line = <WIN>)
{
    $line =~ s/\n$//;
    my @words = split(/ /, $line);
    foreach my $word (@words)
    {
	$rwords{$word} = 1;
    } # foreach
} # while

while (my $line = <IN>)
{
    $line =~ s/\n$//;
    my @words = split(/ /, $line);
    my $first = 1;
    
    # Output every word that isn't in the word list
    foreach my $word (@words)
    {
	if (not defined $rwords{$word})
	{
	    if (not $first)
	    {
		print OUT " ";
	    } # if
	    $first = 0;
	    print OUT $word;
	} # if
    } # foreach
    print OUT "\n";
} # while
