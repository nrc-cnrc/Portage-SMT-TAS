#!/usr/bin/perl -sw
#
# @file unsplit-sentences.pl 
# @brief Undo the work of a sentence splitter.
# 
# @author George Foster
# 
# COMMENTS: 
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
# Copyright 2006, Her Majesty in Right of Canada

print STDERR "unsplit-sentences.pl, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada\n";

$HELP = "
unsplit-sentences.pl sentfile [in [out]]

Undo the work of a sentence splitter: for each number 'n' in sentfile,
join the next n lines from <in> into a single line written to <out>. 
This is intended to work with the -n and -l options of sen-mandarin.pl
(where sentfile is the argument to -l).

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}

my $sentfile = shift || die "Missing <sentfile> arg\n$HELP";
my $in = shift || "-";
my $out = shift || "-";

open(SF, "<$sentfile") or die "Can't open $sentfile for reading\n$HELP";
open(IN, "<$in") or die "Can't open $in for reading\n$HELP";
open(OUT, ">$out") or die "Can't open $out for writing\n$HELP";

while (<SF>) {
   my $nl = $_;
   my $all_lines = "";
   for (my $i = 0; $i < $nl; ++$i) {
      if (!($line = <IN>)) {die "Mismatch between lines in $sentfile and $in!\n";}
      chomp $line;
      if ($all_lines eq "") {$all_lines = $line;}
      else {$all_lines = $all_lines . " " . $line;}
   }
   print OUT $all_lines, "\n";
}

if (<IN>) {die "Mismatch between lines in $sentfile and $in!\n";}
