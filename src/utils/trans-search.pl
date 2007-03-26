#!/usr/bin/perl -s

# trans_search.pl
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# George Foster
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "trans-search.pl, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada\n";

my $HELP = "
trans-search.pl expr

Search for an expression in aligned Hansard text, and print out matching 
sentences along with their translations. Searches are case insensitive,
apply to tokenized text, never match within a word, and are interpreted
as regexps.

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}

my $location_of_aligned_hansard = "/export/echange/corpora/hansard/aligned";
chdir $location_of_aligned_hansard or 
    die "Can't chdir to $location_of_aligned_hansard";
 
my $expr = shift or die "missing <expr> arg\n$HELP\n";

foreach my $en_file (glob "*_en*") {
    my $fr_file = $en_file;
    $fr_file =~ s/_en.al/_fr.al/go;

    if (!(open(EN_FILE, $en_file) && open(FR_FILE, $fr_file))) {
	print STDERR "Can't open file pair $en_file/$fr_file\n";
	next;
    }

    my $en_line;
    my $fr_line;

    while ($en_line = <EN_FILE>) {
	if (!($fr_line = <FR_FILE>)) {
	    print STDERR "file $fr_file too short - skipping\n";
	    next;
	}

	if ($en_line =~ m/(^| )$expr($| )/io ||
            $fr_line =~ m/(^| )$expr($| )/io) {
	    print $en_line, $fr_line, "\n";
	}
    }
    if (defined <FR_FILE>) {
	print STDERR "file $fr_file too long\n";
    }

    # print "$en_file/$fr_file\n";
    
}
