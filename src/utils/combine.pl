#!/usr/bin/perl -sw

# combine.pl
# 
# PROGRAMMER: Aaron Tikuisis
# 
# COMMENTS:
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use IO::File;

print STDERR "combine.pl, NRC-CNRC, (c) 2005 - 2008, Her Majesty in Right of Canada\n";

my $HELP =
"Usage: $0 [-warn] [-force] [file1 [file2 ...]]

Combines the contents of the files, one line at a time.  That is, the output contains line
1 of each file, followed by line 2 of each file, and so on.  

Options:
 -warn	Outputs a warning if the file sizes differ.
 -force	Force file sizes to be treated as the same by outputting blanks after end-of-file.

";

our ($warn, $force, $h, $help);

if (defined $h || defined $help)
{
    print $HELP;
    exit;
} # if

my @files = ();

# Open all files
my $f = shift || "-";
my $F = IO::File->new();
open ($F, "<$f") or die "Cannot open $f for reading";
push @files, $F;

while (my $f = shift)
{
    my $F = IO::File->new();
    open ($F, "<$f") or die "Cannot open $f for reading";
    push @files, $F;
} # while

# Read and print lines
my $keepgoing = 1;
while ($keepgoing)
{
    $keepgoing = 0;	# False until an open file is found
    my @newfiles = ();	# Requeue all the files
    my $linesskipped = 0;
    while (my $file = shift @files)
    {
	my $line = "";
	if ($file != 1 && defined($line = <$file>))
	{
	    print $line;
	    
	    # Check if warning and/or blank lines are needed for previously skipped lines
	    if ($keepgoing == 0 && defined $warn && $linesskipped > 0)
	    {
		print STDERR "Warning: file sizes differ\n";
		undef $warn;
	    } # if
	    while ($keepgoing == 0 && defined $force && $linesskipped > 0)
	    {
		print "\n";
		$linesskipped--;
	    } # while
	    
	    # At least one file is still open so keep going
	    $keepgoing = 1;
	} else
	{
	    # Check if warning and/or blank lines are needed since this file is done
	    if ((defined $force || defined $warn) && not $keepgoing)
	    {
		$linesskipped++;
	    } # if
	    if (defined $force && $keepgoing)
	    {
		print "\n";
	    } # if
	    if (defined $warn && $keepgoing)
	    {
		print STDERR "Warning: file sizes differ\n";
		undef $warn;
	    } # if
	    
	    # Remember that this file is done
	    $file = 1;
	} # if
	push @newfiles, $file;
    } # while
    @files = @newfiles;
} # while
