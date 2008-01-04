#!/usr/bin/perl -sw

# champollion.breakparts.pl Break up merge-alignments output into seperate files.
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

print STDERR "champollion.breakparts.pl, NRC-CNRC, (c) 2005 - 2008, Her Majesty in Right of Canada\n";

my $HELP =
"$0 infile [-out1=outfile1] [-out2=outfile2]

The input file, infile, should be a product of merge_alignment.pl (part of Champollion).
Reads this file and seperates the two languages into seperate files, outfile1 and
outfile2, so that the lines of the two files correspond.

OPTIONS:
infile		The input file (required).
-out1=outfile1	The first output file; if not specified, the suffix of infile is
		substituted by .e to produce outfile1 (eg. sample.txt -> sample.e).
-out2=outfile2	The first output file; if not specified, the suffix of infile is
		substituted by .c to produce outfile2.

";

our ($help, $h);

# Test by comparing the number of lines in the output (english) with the number of occurrences of "<SENT" in the original.

use strict;
use Encode;

our ($out1, $out2);

my $in = shift;

if ($help || $h || not defined $in)
{
    print $HELP;
    exit 0;
} # if

unless (defined $out1)
{
    $out1 = $in;
    if ($out1 =~ /\./)
    {
	$out1 =~ s/\.[^\.]*$/\.e/;
    } else
    {
	$out1 = "$out1.e";
    } # if
} # unless
unless (defined $out2)
{
    $out2 = $in;
    if ($out2 =~ /\./)
    {
	$out2 =~ s/\.[^\.]*$/\.c/;
    } else
    {
	$out2 = "$out2.c";
    } # if
} # unless

my $file = `cat $in`;
Encode::_utf8_off($file);
open(OUT_EN, ">$out1") || die "Can't open $out1 for writing";
open(OUT_CH, ">$out2") || die "Can't open $out2 for writing";

while ($file =~ /<SENT[^>]*>\n+([^\n]*)\n([^<\n]*)\n+<\/SENT>/g)
{
    print OUT_CH $2;
    print OUT_CH "\n";
    print OUT_EN $1;
    print OUT_EN "\n";
}

