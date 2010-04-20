#!/usr/bin/perl -s

# @file strip-parallel-blank-lines.pl 
# @brief Strip parallel blank lines from two line-aligned files. Write output
# to <file1>.no-blanks.
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
printCopyright("strip-parallel-blank-lines.pl", 2005);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP = "
strip-parallel-blank-lines.pl file1 file2

  Strip parallel blank lines from two line-aligned files.
  Write output to <file1>.no-blanks

";

our ($help, $h);

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
my $in1 = shift or die $HELP;
my $in2 = shift or die $HELP;
 
sub getOutputFilename($) {
   my ($out) = @_;
   if ($out =~ s/\.gz$//) {
      $out .= ".no-blanks" . ".gz";
   }
   else {
      $out .= ".no-blanks";
   }
   return $out;
}

zopen(*IN1, "$in1") or die "Can't open $in1 for reading\n";
zopen(*IN2, "$in2") or die "Can't open $in2 for reading\n";
my $out1 = getOutputFilename($in1);
my $out2 = getOutputFilename($in2);
zopen(*OUT1, ">$out1") or die "Can't open $out1 for writing\n";
zopen(*OUT2, ">$out2") or die "Can't open $out2 for writing\n";

my ($line1, $line2);
while ($line1 = <IN1>) {
    if (!($line2 = <IN2>)) {die "file $in2 is too short!\n";}
    if ($line1 !~ /^\s*$/o || $line2 !~ /^\s*$/o) {
        print OUT1 $line1;
        print OUT2 $line2;
    }
}

if (<IN2>) {die "file $in1 is too short!\n";}
