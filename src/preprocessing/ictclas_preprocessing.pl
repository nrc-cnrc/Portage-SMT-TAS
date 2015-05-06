#!/usr/bin/env perl

# @file ictclas_preprocessing.pl
# @brief This script preprocesses Chinese for ICTCLAS.
# 
# @author Boxing Chen & Samuel Larkin
#
# NOTE: Latin1 file encoding
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
printCopyright "ictclas_preprocessing.pl", 2010;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [IN [OUT]]

  Performs ad-hoc pre-processing before ictclas.

";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help => sub { usage },
) or usage "Error: Invalid option(s).";

my $in_file  = shift || "-";
my $out_file = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";



my $xiexian="||";
my $dot="_dot_";
#my $dash="_dash_";

zopen(*IN, "< $in_file") or die "Error: Can't open $in_file for reading: $!\n";
zopen(*OUT, "> $out_file") or die "Error: Can't open $out_file for writing: $!\n";

while (<IN>) {
   chomp;

   s/\x00/ /g;   
   s/\.\.\.\.+/¡­¡­/g;
   s/\¡¤\¡¤\¡¤/¡­/g;
   s/\¡¤\¡¤/\./g;
   s/http\:\/\/ */http\:\/\//g;
   s/https\:\/\/ */https\:\/\//g;
   s/\[__U\+[^\[\]]+\]//g;
   s/\[[^\[\]]+\.gif\]//g;
   s/\[Image\]//g;   
   s/\]\[/\] \[/g;
   s/([\d\.]+ ){4,}//g;
   s/ (\d+)\s*\.\s*(\d+) /$1\.$2/g;
   
   my @word=split(/ +/);
   for (my $i=0;$i<@word;$i++) {
      if ($word[$i]=~/\*/){
         $word[$i]=~s/\*/ \* /g;
      }
      if ($word[$i]=~/\</){
         $word[$i]=~s/\</ \</g;
      }
      if ($word[$i]=~/\>/){
         $word[$i]=~s/\>/\> /g;
      }
      if ($word[$i]=~/[a-zA-Z\@\.\%\d\$\-\/\\\|\#\^\&\*\(\)\_\+\=|[\]\{\}\/\<\>\,\:\;\"\'\~\`\!]+/){         
         $word[$i]=" $word[$i] ";
      }
      if ($word[$i]=~/http/){
         $word[$i]=~s/\:(\d+)(\/*)/ \: $1/g;
      }
      else{         
         $word[$i]=~s/\//$xiexian/g;
         $word[$i]=~s/([a-zA-Z]+)\.([a-zA-Z]+)/$1$dot$2/g;
         #$word[$i]=~s/\-/$dash/g;
      }
      if ($word[$i]=~/(\d+\.\d+\.\d+)\-(\d+\.\d+\.\d+)/){
         $word[$i]=~s/(\d+\.\d+\.\d+)\-(\d+\.\d+\.\d+)/ $1 \- $2 /g;
      }
      if ($word[$i]=~/(\d+\.)(\d+\.)+/){
         $word[$i]=~s/\./$dot/g;
      }
      if ($word[$i]=~/([a-zA-Z]+\.\d+)+/){
         $word[$i]=~s/\./$dot/g;
      }
      if ($word[$i]=~/([a-zA-Z]\.)([a-zA-Z]\.)+/){
         $word[$i]=~s/\./$dot/g;
      }
   }
   my $out=join("",@word);
   $out=~s/\=\=+/\=\=/g;
   $out=~s/__+/__/g;
   $out=~s/\-\-+/ \-\- /g;
   $out=~s/\'\'/\¡±/g;
   $out=~s/\`\`/\¡°/g;
   $out=~s/\+/ \+ /g;   

   $out=~s/ +/ /g;
   $out=~s/^ | $//g;

   print OUT $out, "\n";
}

close(IN);
close(OUT);

