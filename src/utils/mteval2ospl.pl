#!/usr/bin/perl -s

# @file mteval2ospl.pl 
# @brief Convert NIST MT format to one-sentence-per-line.
# 
# @author George Foster
# 
# COMMENTS: 
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "mteval2ospl.pl, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada\n";

my $HELP = "
mteval2ospl.pl [-G][-g genre] file

Convert a NIST MT eval formatted into one-sentence-per line output, writing one
file per document in the input file. (To just extract all segments in order,
use mteval2ospl.sed).

Options:

-g  Write only documents identified by tags as being of a given genre.
-G  Write a list of segment id's along with the genre of each segment to stdout.
    This option is not compatible with -g.

";

our ($help, $h, $g, $G);

if ($help || $h) {
    print $HELP;
    exit 0;
}

$g = "" unless defined $g;

if ($g ne "" && $G) {die "Can't specify both -g and -G\n";}
 
my $in = shift || die "Missing file argument\n$HELP";
my $out = shift || "-";
 
open(IN, "<$in") or die "Can't open $in for reading\n";

undef $/;
my $content = <IN>;
close IN;

my $segno = 0;

while ($content =~ /<doc([^>]*)>(.*?)<\/doc>/isgo) {

    my $attrs = $1;
    my $guts = $2;
    
    my ($docid, $sysid, $genre) = ("","","");
    if ($attrs =~ /docid\s*=\s*\"([^\"]+)\"/io) {$docid = $1;}
    if ($attrs =~ /sysid\s*=\s*\"([^\"]+)\"/io) {$sysid = $1;}
    if ($attrs =~ /genre\s*=\s*\"([^\"]+)\"/io) {$genre = $1;}

    next if ($g ne "" && $genre ne $g);

    if ($docid eq "") {die "Missing docid tag";}

    if (!$G) {
       my $docname = "$docid$sysid";
       open(OUT, ">$docname") or die "Can't open $docname for writing\n";
    }

    while ($guts =~ /<seg[^>]*>\s*(.*?)\s*<\/seg>/isgo) {
        my $result = $1;
        $result =~ s/\r\n/ /g;  # removes the accidental windows newline
	if ($G) {print ($segno++, "\t", $genre, "\n");}
        else {print OUT "$result\n";}
    }
    close(OUT);
}
