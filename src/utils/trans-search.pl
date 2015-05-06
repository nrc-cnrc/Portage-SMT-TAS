#!/usr/bin/perl -s

# @file trans_search.pl 
# @brief Search for an expression in aligned Hansard text, and print out
# matching sentences along with their translations.
# 
# @author George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

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
printCopyright "trans-search.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


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
    die "Error: Can't chdir to $location_of_aligned_hansard";
 
my $expr = shift or die "Error: Missing <expr> arg\n$HELP\n";

foreach my $en_file (glob "*_en*") {
    my $fr_file = $en_file;
    $fr_file =~ s/_en.al/_fr.al/go;

    if (!(open(EN_FILE, $en_file) && open(FR_FILE, $fr_file))) {
	print STDERR "Warning: Can't open file pair $en_file/$fr_file\n";
	next;
    }

    my $en_line;
    my $fr_line;

    while ($en_line = <EN_FILE>) {
	if (!($fr_line = <FR_FILE>)) {
	    print STDERR "Warning: File $fr_file too short - skipping\n";
	    next;
	}

	if ($en_line =~ m/(^| )$expr($| )/io ||
            $fr_line =~ m/(^| )$expr($| )/io) {
	    print $en_line, $fr_line, "\n";
	}
    }
    if (defined <FR_FILE>) {
	print STDERR "Warning: File $fr_file too long\n";
    }

    # print "$en_file/$fr_file\n";
    
}
