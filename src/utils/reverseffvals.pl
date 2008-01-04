#!/usr/bin/perl -sw

# reverseff.pl Reverses the words in each line of input of -ffvals text
#
# PROGRAMMER: Matthew Arnold
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;

print STDERR "reverseff.pl, NRC-CNRC, (c) 2005 - 2008, Her Majesty in Right of Canada\n";

my $HELP =
"Usage: $0 [infile [outfile]]

Reverses the output of ffvals (\"phrase1\" (params) \"phrase1\" (params) etc...) into the forward version of the 
output (ie reverses phrase orders and words inside phrases).

Options:
infile  The input file; if not specified, standard input is used.
outfile The output file; if not specified, standard output is used.

";

our ($help, $h);

if ($help || $h)
{
    print $HELP;
	exit 0;
} # if

my $in = shift || "-";
my $out = shift || "-";

open(IN, "<$in") || die "Can't open $in for reading";
open(OUT, ">$out") || die "Can't open $out for writing";

# Enable immediate flush when piping
select(OUT); $| = 1;
while (my $line = <IN>)
{
	my @body = ();
    $line =~ s/\n//;
    while ($line =~ /\"((?:[^\"]|\\\")*)\" \(([^\(\)]*)\)/g)
	{
		my $phrase = $1;
		my $vals = "(" . $2 . ")";
		my @words = split(/ /, $phrase);
		@words = reverse @words;
		#words now contains the correct ordering of the phrase
		$phrase = "\"" . join(' ', @words) . "\"";
		#phrase should now have the words correctly

		my $pair = $phrase . " " . $vals;
		my @tempBody = ($pair, @body);
		@body = @tempBody;
	}
	
	
    print OUT "@body\n";
} # while
						
