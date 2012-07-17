#!/usr/bin/env perl

# @file chinese_segmenter.pl
# @brief Segment utf8 Chinese text in "words".
#
# NB: chinese_segmenter.pl assumes that dictfile and infile are encoded without
# error using Unicode UTF-8.
# i.e., it assumes that detection of invalid codes and any needed
# conversions have been done beforehand.
#
# This is a derived work based on mansegment.pl as distributed by the
# LDC.  The source has been completely re-organized and numerous
# improvements made to the performance and readability.  Almost all of
# of the code is different but the functionality and the skeleton of
# of the logic is the same.  In particular, the algorithm used for the
# dynamic programming is loosely similar and the file format of the
# dictionary is the same.
#
# This rewrite completed by Howard Johnson
# Technologies langagieres interactives / Interactive Language Technologies
# Technologies de l'information et des communications /
#    Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
#
# Terminology:
# zi: A Chinese character, also sometimes called hanzi (or kanji by
#     Japanese).  Often a zi can function as a word or morpheme as
#     the concepts are applied to other languages but many words and
#     morphemes are made up of a number of zi.  In Chinese, almost
#     without exception a zi is one syllable long.
# ci: A Chinese word or morpheme.  Classical Chinese did not consider
#     explicitly such things but all modern dialects have a large
#     number of non-compositional sequences of zi that are listed in
#     cidian (or word dictionaries).
# juzi: A Chinese sentence.  More or the less the same as sentences
#     in other languages, although often multiple sentences may be
#     used to express a very complex English sentence.
# HSK: An series of examinations offered internationally to establish
#     the proficiency in Chinese.  One part of the syllabus is a list
#     of ci that must be mastered for each of 4 levels of competancy.
#     Of course, there is much more required to pass.
#====================

#====================
# The original header for LDC's mansegment.perl was as follows:
#

###############################################################################
# This software is being provided to you, the LICENSEE, by the Linguistic     #
# Data Consortium (LDC) and the University of Pennsylvania (UPENN) under the  #
# following license.  By obtaining, using and/or copying this software, you   #
# agree that you have read, understood, and will comply with these terms and  #
# conditions:                                                                 #
#                                                                             #
# Permission to use, copy, modify and distribute, including the right to      #
# grant others the right to distribute at any tier, this software and its     #
# documentation for any purpose and without fee or royalty is hereby granted, #
# provided that you agree to comply with the following copyright notice and   #
# statements, including the disclaimer, and that the same appear on ALL       #
# copies of the software and documentation, including modifications that you  #
# make for internal use or for distribution:                                  #
#                                                                             #
# Copyright 1999 by the University of Pennsylvania.  All rights reserved.     #
#                                                                             #
# THIS SOFTWARE IS PROVIDED "AS IS"; LDC AND UPENN MAKE NO REPRESENTATIONS OR #
# WARRANTIES, EXPRESS OR IMPLIED.  By way of example, but not limitation,     #
# LDC AND UPENN MAKE NO REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR   #
# FITNESS FOR ANY PARTICULAR PURPOSE.                                         #
###############################################################################
# mansegment.perl Version 1.1
# Run as: mansegment.perl [dictfile] < infile > outfile
# A Chinese segmenter for both GB and BIG5 as long as the cooresponding 
# word frequency dictionary is used.
#
# Written by Zhibiao Wu at LDC on April 12 1999
# Modified by Xiaoyi Ma at LDC, March, 2003
# Change of v1.1:
# - simplified code
# - regenerated database to be compatible with perl5
#
# Algorithm: Dynamic programming to find the path which has the highest 
# multiple of word probability, the next word is selected from the longest
# phrase.
#
# dictfile is a two column text file, first column is the frequency, 
# second column is the word. The program will change the file into a dbm 
# file in the first run. So be sure to remove the dbm file if you have a
# newer version of the text file.
##############################################################################

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
printCopyright "chinese_segmenter.pl", 2008;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [IN [OUT]]

  A Chinese segmenter.

Options:

  -h(elp)       print this help message
  -dic DIC      dictionary file [manseg.fre].
";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
   "dic=s"     => \my $dictfile,
) or usage;

my $in  = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";



unless (defined($dictfile)) {
   my $DICTPATH = ".";
   if ( $0 =~ /\// ) {
      $DICTPATH = "$1/lib" if ( $0 =~ m#(.+)/bin/[^/]+# );
   }

   $dictfile = "$DICTPATH/manseg.fre";
}


my %freq;
my %log_freq;
my %max_ci_len;
my $total_freq = 0;
my $log_total_freq = 0;
my $prob_iv = 0.9998;
my $log_prob_oov = -log( ( 1 - $prob_iv ) / 1000 );

#==========
# Read in the ci dictionary.  A ci (or danci) is a non-compositional
# frequently occuring combination of zi (hanzi) that are very much
# like a word (or morpheme) grammatically.
#
die "Unable to find dictionary file manseg.fre" unless(-e $dictfile);
open( DICT, "< :encoding(UTF-8)", $dictfile ) || die "Dictonary file $dictfile not found";
while ( <DICT> ) {
   chomp;
   s/^ *//;
   my( $freq, $ci ) = split;
   $log_freq{ $ci } = log( $freq );
   my $first_zi = substr( $ci, 0, 1 );
   if (
         ! defined( $max_ci_len{ $first_zi } )
         || $max_ci_len{ $first_zi } < length( $ci )
      ) {
      $max_ci_len{ $first_zi } = length( $ci );
   }
   $total_freq += $freq;
}
close( DICT );
$log_total_freq = log( $total_freq / $prob_iv );
#
#==========

my $first_node;
my $free_node = 0;
my @next;

zopen(*IN, "< $in") or die "Can't open $in for reading: $!\n";
zopen(*OUT, "> $out") or die "Can't open $out for writing: $!\n";

binmode( IN,  ":encoding(UTF-8)" );
binmode( OUT, ":encoding(UTF-8)" );

while ( <IN> ) {
   s/[\r]?[\n]?$//;
   print OUT manseg( $_ ), "\n";
}

close(IN);
close(OUT);





sub manseg {
   my ( $content ) = @_;
   my @segs;
#  while ( $content && $content =~ s/^([^\p{Han}]*)([\p{Han}\pZ]*)// ) {
# JHJ Apr 5 2007: Change to cause all white space to be treated as
#                 Non-Hanzi and thus preserve all pre-existing token
#                 boundaries.  Otherwise, HanZi are all squished together
#                 before segmentation.
   while ( $content && $content =~ s/^([^\p{Han}]*)([\p{Han}]*)// ) {
      my $non_hanzi_segment = $1;
      my $hanzi_segment = $2;
      $hanzi_segment =~ s/[\pZ]//g;
      my $nh = process_non_hanzi_segment( $non_hanzi_segment );
      my $hz = process_hanzi_segment( $hanzi_segment );
      if ( $nh ne '' ) { push( @segs, $nh ); }
      if ( $hz ne '' ) { push( @segs, $hz ); tabulate_segs( $hz ); }
   }
   my $joined = join( "\000", @segs );
   $joined =~ s/^[\000]//;
   $joined =~ s/[\000]$//;
   $joined =~ s/([\n\pZ])[\000]/$1/g;
   $joined =~ s/[\000]([\n\pZ])/$1/g;
   $joined =~ s/[\000]/ /g;
   return $joined;
}

sub crunch {
   my ( $str ) = @_;
   $str =~ s/ //g;
   return $str;
}

sub crunch2 {
   my ( $str ) = @_;
   return '[' . crunch( $str ) . ']';
}

sub process_non_hanzi_segment {
   my ( $segment ) = @_;
#  $segment =~ s/[\pC]+//g;
#  $segment =~ s/([\pL]+)/ $1 /g;
#  $segment =~ s/([\pN]+)/ $1 /g;
#  $segment =~ s/([\pP])/ $1 /g;
   $segment =~ s/[\pZ]+/ /g;
   $segment =~ s/^ //;
   $segment =~ s/ $//;
#  $segment =~ s/\[ \[ \[ U \+([0-9a-f ]*)\] \] \]/&crunch($&)/eg;
#  $segment =~ s/\[ \[ U \+([0-9a-f ]*)\] \]/&crunch2($&)/eg;
#  $segment =~ s/\[ \[ \[ \*([0-9a-f ]*)\] \] \]/&crunch($&)/eg;
#  return "[[[" . $segment . "]]]";
   return $segment;
}

sub process_hanzi_segment {
   my ( $segment ) = @_;
   my @segment_offset;
   my @log_rel_freq;
   my @tokenized;

   $first_node = 1;
   $free_node = 0;
   $next[ 0 ] = 0;
   $segment_offset[ 1 ] = 0;
   $log_rel_freq[ 1 ] = 0.0;
   $tokenized[ 1 ] = "";
   $next[ 1 ] = 0;
   my $node_hi_water = 1;
   my $segment_len = length( $segment );
   my $flag = 1;

   while ( $flag ) {

#==========
# Find the first node whose segment_offset is less than the length
# of the segment.  This will have the largest relative frequency.
#
      my $current;
      my $previous_node;
      for (
            $current = $first_node
            ; $current && $segment_offset[ $current ] == $segment_len
            ; $current = $next[ $current ]
          ) {
         $previous_node = $current;
      }
#
#==========

#==========
# current is now the node removed from the node list (or not).
# Remove it from the list and see if there is an extension from it.
#
      if ( $current ) {
         if ( $current == $first_node ) {
            $first_node = $next[ $first_node ];
         }
         else {
            $next[ $previous_node ] = $next[ $current ];
         }

#==========
# look up the longest ci that occurs in the dictionary and will
# fit the remaining portion of the segment.
#
         my $first_zi = substr( $segment, $segment_offset[ $current ], 1 );
         if ( ! defined( $log_freq{ $first_zi } ) ) {
            $log_freq{ $first_zi } = $log_prob_oov - $log_total_freq;
         }
         my $max_ci_len = 1;
         if ( defined $max_ci_len{ $first_zi } ) {
            $max_ci_len = $max_ci_len{ $first_zi };
            if ( $max_ci_len > $segment_len - $segment_offset[ $current ] ) {
               $max_ci_len = $segment_len - $segment_offset[ $current ];
            }
         }
#
# at this point: ci, ci_len, freq_ci are defined
#==========

         for ( my $ci_len = $max_ci_len; $ci_len > 0 ; --$ci_len ) {
            my $ci = substr( $segment, $segment_offset[ $current ], $ci_len );
            if ( defined( $log_freq{ $ci } ) ) {
               my $log_freq_ci = $log_freq{ $ci };

#==========
# Create a new node, extending the current one.
# current can then be scavenged.
#
               my $new_node;
               if ( $free_node ) {
                  $new_node = $free_node;
                  $free_node = $next[ $free_node ];
#            if ( check_in_list( $new_node ) ) {
#              print STDERR "error: new_node still in list\n";
#            }
               }
               else {
                  $new_node = ++$node_hi_water;
               }
               $segment_offset[ $new_node ] = $segment_offset[ $current ] + $ci_len;
               $log_rel_freq[ $new_node ]
                  = $log_rel_freq[ $current ] - $log_freq_ci + $log_total_freq;
               $tokenized[ $new_node ] = $tokenized[ $current ] . " " . $ci;
#
#==========

#==========
# See if we need to insert the new node:
# CASE 1: There is a node with the same segment_offset and a larger
#   relative frequency.  Forget about the new node.
# CASE 2: There is a node with the same segment_offset and a smaller
#   relative frequency.  Delete this node and insert the new node.
# CASE 3: There is no node with this segment_offset.  Insert the new
#   node.
#
               my $need_insert = 1;
               my $index;
               for (   $index = $first_node
                     ; $index
                     ; $index = $next[ $index ]
                   ) {
                  if ( $segment_offset[ $index ] == $segment_offset[ $new_node ] ) {
                     if ( $log_rel_freq[ $index ] <= $log_rel_freq[ $new_node ] ) {
                        $need_insert = 0;
                        $next[ $new_node ] = $free_node;
                        $free_node = $new_node;
                     }
                     else {
                        if ( $index == $first_node ) {
                           $first_node = $next[ $index ];
                        }
                        else {
                           $next[ $previous_node ] = $next[ $index ];
                        }
#==========
# Here we are deleting the node that we are currently working on.
# The increment on the loop will be garbled unless we do corrective
# action.  One easy thing is to terminate the loop.  There shouldn't
# be any more entries with the same segment_offset anyway.
#
                        $next[ $index ] = $free_node;
                        $free_node = $index;
                        $index = 0;
#
#==========
                     }
                  }
                  $previous_node = $index;
               }
               if ( $need_insert ) {
                  for (   $index = $first_node
                        ;    $index
                        && $log_rel_freq[ $index ] < $log_rel_freq[ $new_node ]
                        ; $index = $next[ $index ]
                      ) {
                     $previous_node = $index;
                  }
                  if ( $index == $first_node ) {
                     $next[ $new_node ] = $first_node;
                     $first_node = $new_node;
                  }
                  else {
                     $next[ $new_node ] = $index;
                     $next[ $previous_node ] = $new_node;
                  }
               }
            }
         }
#
#==========

#
# or not
#
         $next[ $current ] = $free_node;
         $free_node = $current;
      }
      else {
         $flag = 0;
      }
   }
#
#==========

   if ( $first_node ) {
      $tokenized[ $first_node ] =~ s/^ *//;
      return $tokenized[ $first_node ];
   }
   else {
      print STDERR "Error: $. $segment.\n";
   }
}

sub tabulate_segs {
   my ( $hz ) = @_;
   my @segs = split( " ", $hz );
   foreach my $i ( @segs ) {
      $freq{ $i }++;
   }
}

sub compare_freq_keys {
   my $result = $freq{ $b } <=> $freq{ $a };
   if ( $result != 0 ) {
      return $result;
   }
   else {
      return $a cmp $b;
   }
}

sub report_freq {
   my $count = 0;
   foreach my $i ( keys %freq ) {
      $count += $freq{ $i };
   }
   open( REPORT, "> :encoding(UTF-8)", "C04_report.txt" );
   print REPORT "$count\tTotal\n";
   foreach my $i ( sort compare_freq_keys keys( %freq ) ) {
      print REPORT $i, "\t", $freq{ $i }, "\n";
   }
   close( REPORT );
}

sub print_list {
   audit_list();
   my $i;
   for ( $i = $first_node; $i; $i = $next[ $i ] ) {
      print STDERR $i, " ";
   }
   print STDERR "\n";
}

my $cnt = 0;
sub audit_list {
   my $i1;
   my $i2;
   for (
         $i1 = $first_node, $i2 = $next[ $first_node ]
         ; $i2 && $i1 != $i2
         ; $i1 = $next[ $i1 ], $i2 = $next[ $next[ $i2 ] ]
       ) {
   }
   if ( $i1 && $i1 == $i2 ) {
      print STDERR "Loop in node list\n";
      exit;
   }
   if ( ++$cnt > 10000 ) {
      print STDERR "Exit on count\n";
      exit;
   }
}

