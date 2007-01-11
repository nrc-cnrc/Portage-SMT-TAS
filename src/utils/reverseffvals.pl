#!/usr/bin/perl -sw

# reverseff.pl Reverses the words in each line of input of -ffvals text
#
# PROGRAMMER: Matthew Arnold
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use strict;

print STDERR "reverseff.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

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
						
