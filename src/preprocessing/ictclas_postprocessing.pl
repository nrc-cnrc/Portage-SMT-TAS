#!/usr/bin/env perl
# $Id$

# @file ictclas_postprocessing.pl
# @brief This script postprocesses ICTCLAS output.
#
# @author Boxing Chen & Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

#######################################################################################################

use warnings;
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
printCopyright "ictclas_postprocessing.pl", 2010;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [IN [OUT]]

  Performs ad-hoc post-processing after ictclas.

";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help => sub { usage },
) or usage;

my $in_file  = shift || "-";
my $out_file = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";



my $count = 0;

zopen(*IN, "< $in_file") or die "Can't open $in_file for reading: $!\n";
zopen(*OUT, "> $out_file") or die "Can't open $out_file for writing: $!\n";

while (<IN>) {
   chomp;
   ++$count;
   #print STDERR "$count\n";
   my @word=split(/ +/);
   for (my $i=0;$i<@word;$i++){
      if ($word[$i]=~/(.+)\/[^\/]*$/){
         $word[$i]=$1;
      }
   }
   my $out = join(" ",@word);

   $out=~s/两日/ 两日/g;
   $out=~s/部 长沙 万/部长 沙万/g;
   
   $out=~s/_dot_/\./g;
   #$out=~s/_dash_/ \- /g;
   $out=~s/——/\-/g;
   $out=~s/\|\|/\//g;

   $out=~s/([^a-zA-Z\@\d\.\%\/\-\<\>]*)([a-zA-Z\@\d\.\%\/\-\<\>]+)([^a-zA-Z\@\d\.\%\/\-\<\>]*)/ $1 $2 $3/g;

   $out=~s/·/ · /g; #sperator of transliterating name
   $out=~s/http \: */http\:/g;
   $out=~s/\:http/\: http/g;

   $out=~s/https \: */https\:/g;
   $out=~s/\:https/\: https/g;
   
   if ($out=~/ (http[a-zA-Z0-9\@\d\.\?\_\%\/\-\<\>\s\:\=]+) /){
      my $match1=$1;
      my $match2=$1;
      $match2=~s/ //g;
      $out=~s/$match1/$match2/g;
   }
   $out=~s/\.com([^\.\/]+)/\.com $1/g;
   $out=~s/\.org([^\.\/]+)/\.org $1/g;
   $out=~s/\.net([^\.\/]+)/\.net $1/g;
   
   $out=~s/http/ http/g;

   $out=~s/ +/ /g;
   $out=~s/^ | $//g;
   print OUT $out, "\n";
}

close(IN);
close(OUT);

