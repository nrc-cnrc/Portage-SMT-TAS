#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;
#
# @file split-listed.pl 
# @brief Like unix split, but split into a given list of output files,
# contained inlistfile.
# 
# @author George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
# Copyright 2006, Her Majesty in Right of Canada

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
printCopyright "split-listed.pl", 2006;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


$HELP = "
split-listed.pl [-d=outdir] listfile [infile]

Like unix split, but split into a given list of output files, contained in
listfile. Each line in listfile is in the format 'numlines outfile', and
directs that the next numlines lines from infile are appended to outfile.
If numlines is < 0, all remaining lines are written to outfile.

Options:

-d Write output files in directory outdir [current dir]

";

our ($help, $h, $outdir);

if ($help || $h) {
    print $HELP;
    exit 0;
}

$d = "." unless defined $d;

my $listfile = shift || die "Error: Missing <listfile> arg\n$HELP";
my $in = shift || "-";

open(SF, "<$listfile") or die "Error: Can't open $listfile for reading\n";
open(IN, "<$in") or die "Error: Can't open $in for reading\n";

while (<SF>) {
    my ($nl, $out) = split;
    open(OUT, ">>$d/$out") or die "Error: Can't open $d/$out for writing\n";
    
    for (my $i = 0; $i < $nl || $nl < 0; ++$i) {
	if (!($line = <IN>)) {
           if ($nl >= 0) {die "Error: No more lines left in input $in!\n";}
           else {last;}
        }
	print OUT $line;
    }
    
    close OUT;
}

if (<IN>) {die "Error: No more lines left in listfile $listfile!\n";}
