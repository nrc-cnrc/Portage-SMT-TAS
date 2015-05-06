#!/usr/bin/perl -sw

# @file reverseffvals.pl 
# @brief Reverse the words in each line of input of -ffvals text.
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
printCopyright "reverseffvals.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP =
"Usage: reverseffvals.pl [infile [outfile]]

Reverse the output of ffvals (\"phrase1\" (params) \"phrase1\" (params) etc...)
into the forward version of the output (i.e. reverse phrase orders and words
inside phrases).

Options:
infile  Input file; if not specified, standard input is used.
outfile Output file; if not specified, standard output is used.

";

our ($help, $h);

if ($help || $h)
{
    print $HELP;
	exit 0;
} # if

my $in = shift || "-";
my $out = shift || "-";

open(IN, "<$in") || die "Error: Can't open $in for reading";
open(OUT, ">$out") || die "Error: Can't open $out for writing";

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
						
