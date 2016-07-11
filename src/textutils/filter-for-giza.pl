#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;
#
# @file filter-for-giza.pl 
# @brief Produce a pair of line-aligned files for giza.
#
# Take as input a line-aligned bilingual corpus in the form of a pair
# of source and target files, and produce a corresponding pair of files
# "f.giza" and "e.giza", in which the following pairs of lines have been
# eliminated:
# - either line is empty
# - one line exceeds 100 words
# - one line is  at least twice as long as the other
# - either line contains any of the forbidden characters "#", "{" or "}" 
#
# @author Michel Simard but don't blame me
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
printCopyright "filter-for-giza.pl", 2006;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP = "
Usage: filter-for-giza.pl {options} src_file tgt_file

Filter line-aligned bilingual corpus src_file - tgt_file for usage
with GIZA (eliminate pairs that are empty or too long or too different
in size or that contain illegal characters). Result goes to files f.giza
and e.giza (but see -outsrc and -outtgt options).

Options:
  -verbose      Be verbose [don't]
  -h, -help     Print this message and exit
  -outsrc=X     Output source-language file is X [f.giza]       
  -outtgt=X     Output target-language file is X [e.giza]
  -max=M        Max number of words per line [100]
  -ratio=R      Max word ratio per pair; should be > 1 [2]
  -rejects      Warn of each rejected pair
";

our ($verbose, $outsrc, $outtgt, $max, $ratio, $rejects, $h, $help);

if ($h or $help) {
    print $HELP;
    exit 0;
}


$verbose = 0 unless defined $verbose;
$rejects = 0 unless defined $rejects;
$max = 100 unless defined $max;
$ratio = 2 unless defined $ratio;
my $min_ratio = 1/$ratio;
my $max_ratio = $ratio;

my $srcfile = shift or die "Error: Missing arg: source file";
my $tgtfile = shift or die "Error: Missing arg: target file";

$outsrc = "f.giza" unless defined $outsrc;
$outtgt = "e.giza" unless defined $outtgt;

verbose("[Reading in files $srcfile and $tgtfile]\n");

open(SRC, "<$srcfile") or die "Error: Can't open source file $srcfile";
open(TGT, "<$tgtfile") or die "Error: Can't open target file $tgtfile";

verbose("[Writing output to $outsrc and $outtgt]\n");

open(OUTSRC, ">$outsrc") or die "Error: Can't open output source file $outsrc";
open(OUTTGT, ">$outtgt") or die "Error: Can't open output target file $outtgt";

my $line_count = 0;
my $out_count = 0;
while (my $src_line = <SRC>) {
    die "Error: Unequal number of source and target lines" if eof(TGT);
    my $tgt_line = <TGT>;
    ++$line_count;
    chop $src_line;
    chop $tgt_line;

    my @words = split(/\s+/o, $src_line);
    my $src_words = scalar(@words);
    @words = split(/\s+/o, $tgt_line);
    my $tgt_words = scalar(@words);
    my $R = $src_words / $tgt_words;
    my $forbidden = ($src_line =~ /[\#\{\}]/o) or ($tgt_line =~ /[\#\{\}]/o);
    
    if (!$forbidden 
        && (0 < $src_words) && ($src_words <= $max)
        && (0 < $tgt_words) && ($tgt_words <= $max)
        && ($min_ratio < $R) && ($R < $max_ratio)) {
        ++$out_count;
        print OUTSRC $out_count, ' ', $src_line, "\n";
        print OUTTGT $out_count, ' ', $tgt_line, "\n";
    } elsif ($rejects) {
        warn("reject: $line_count: forbidden=$forbidden, $src_words / $tgt_words = $ratio\n");
    }
}

verbose("[Done (read $line_count pairs of lines, output $out_count)]\n");

die "Error: Unequal number of source and target lines"
    unless eof(TGT);

close OUTSRC;
close OUTTGT;
close SRC;
close TGT;

sub verbose { printf STDERR @_ if $verbose; }
