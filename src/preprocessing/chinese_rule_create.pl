#!/usr/bin/env perl

use strict;
use warnings;
require 5.8.5;
use utf8;

# $Id$
#
# trans_gb_en.pl - convert the marked up gb files to Canoe format
#
# Usage: trans_gb_en.pl <input file>
#
# Programmer: Song Qiang Fang - overhauled by Howard Johnson in 2006
#
# Copyright (c) 2004, 2005, 2006, Conseil national de recherches Canada / National Research Council Canada
#
# This software is distributed to the GALE project participants under the terms
# and conditions specified in GALE project agreements, and remains the sole
# property of the National Research Council of Canada.
#
# For further information, please contact :
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html


# format:  在 <NPP english='January 1st, 1998|Jan. 1st, 1998|January first, 1998' prob='0.8|0.9|0.5'>一九九八年一月一日</NPP>
#
#As a test, <DATE> and <NUM> in gb will be traslated without probability.
#standard foramt: MM DD ,ie  一月一日===> january 1
#      YY MM, ie  一九九八年一月==> january 1998
#      YY MM DD, ie 一九九八年一月一日==>january 1,1998

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
printCopyright "chinese_rule_create.pl", 2008;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [IN [OUT]]

  Convert the marked up gb files to Canoe format.

Options:

  -h(elp)       print this help message
";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
) or usage;

my $in  = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";



my %gb_en_month1 = (
  "1月" => "january",
  "2月" => "february",
  "3月" => "march",
  "4月" => "april",
  "5月" => "may",
  "6月" => "june",
  "7月" => "july",
  "8月" => "august",
  "9月" => "september",
  "10月" => "october",
  "11月" => "november",
  "12月" => "december"
);

my %gb_en_month2 = (
  "1" => "january",
  "2" => "february",
  "3" => "march",
  "4" => "april",
  "5" => "may",
  "6" => "june",
  "7" => "july",
  "8" => "august",
  "9" => "september",
  "10" => "october",
  "11" => "november",
  "12" => "december"
);

my %gb_en_day = (
  "1" => "1st",
  "2" => "2nd",
  "3" => "3rd",
  "4" => "4th",
  "5" => "5th",
  "6" => "6th",
  "7" => "7th",
  "8" => "8th",
  "9" => "9th",
  "10" => "10th",
  "11" => "11th",
  "12" => "12th",
  "13" => "13th",
  "14" => "14th",
  "15" => "15th",
  "16" => "16th",
  "17" => "17th",
  "18" => "18th",
  "19" => "19th",
  "20" => "20th",
  "21" => "21st",
  "22" => "22nd",
  "23" => "23rd",
  "24" => "24th",
  "25" => "25th",
  "26" => "26th",
  "27" => "27th",
  "28" => "28th",
  "29" => "29th",
  "30" => "30th",
  "31" => "31st"
);

my %SSS = (
  "0" => "[[error]]",
  "1" => "thousand",
  "2" => "million",
  "3" => "billion",
  "4" => "trillion",
  "5" => "quadrillion",
  "6" => "quintillion",
);

my $date_100 = "xx年";
my $date_010 = "xx月";
my $date_001 = "xx日";
my $date_111 = $date_100 . $date_010 . $date_001;
my $date_110 = $date_100 . $date_010;
my $date_011 = $date_010 . $date_001;
my $date_100_dai = "xx年代";

my $percent_f = "百分之";
my $percent_b = "%|％";
my $percent = "($percent_f)|$percent_b";
my $us_dollar = "美元";
my $more_wan = "多[ ]*万";
my $more_yi = "多[ ]*亿";
my $more_wan_yi = "($more_wan)|($more_yi)";

my $date_mark_up = "<DATE [^>]*[>][^<]*<\/DATE>";
my $num_mark_up = "<NUM [^>]*[>][^<]*<\/NUM>";

sub tran_num {
   my ( $number ) = @_;
   unless ( $number =~ m/(.*)<NUM value=\"(.*)\">(.*)<\/NUM>(.*)/ ) {
      die "error in this num expression!";
   }
   my $pre_stuff = $1;
   my $save_num = $2;
   my $orig_num = $3;
   my $post_stuff = $4;
   if ( $save_num =~ /^[\d.]*$/ ) {

      if ( $number =~ /$more_wan_yi/ ) {
         if( $number =~ /$more_wan/ ) {
            $save_num *= 10000;
         } elsif ( $number =~ /$more_yi/ ) {
            $save_num *= 100000000;
         }
      }
      my $num_with_comma = commify( $save_num );
      my $num_english = convert_comma( $num_with_comma );
      if ( $number =~ m/$percent/ ) {
         $num_english .= " %";
         if ( $number =~ m/$percent_f/ ) {
            $orig_num = $percent_f . " " . $orig_num;
         } elsif ( $number =~ m/$percent_b/ ) {
            $orig_num = $orig_num . " " . $&;
         } else{
            die " error in percentage expression";
         }
      } elsif ( $number =~ /$more_wan_yi/ ) {
         $num_english = "more than " . $num_english;
         if ( $number =~ /$more_wan/ ){
            $orig_num .= " $&";
         } elsif ( $number =~ /$more_yi/ ) {
            $orig_num .= " $&";
         } else {
            die " error in duo wan or duo yi expression";
         }
      } elsif ( $number =~ m/$us_dollar/ ) {
         $num_english .= " us dollars";
         $orig_num .= " $us_dollar";
      }
      return " <NPP english=\"$num_english\" prob=\"1.0\"> $pre_stuff $orig_num $post_stuff </NPP> ";
   } elsif ( $save_num =~ /^> ([\d.]*)([ %dolars]*)$/ ) {
      my $num_with_comma = commify( $1 );
      my $num_english = convert_comma( $num_with_comma );
      return " <NPP english=\"more than $num_english$2\" prob=\"1.0\"> $pre_stuff $orig_num $post_stuff </NPP> ";
   } elsif ( $save_num =~ /^([\d.]*)([ %dolars]*)$/ ) {
      my $num_with_comma = commify( $1 );
      my $num_english = convert_comma( $num_with_comma );
      return " <NPP english=\"$num_english$2\" prob=\"1.0\"> $pre_stuff $orig_num $post_stuff </NPP> ";
   } elsif ( $save_num =~ /^([\d]*) or ([\d.]*)([ %dolars]*)$/ ) {
      my $num_with_comma = commify( $2 );
      my $num_english = convert_comma( $num_with_comma );
      return " <NPP english=\"$1 or $num_english$3\" prob=\"1.0\"> $pre_stuff $orig_num $post_stuff </NPP> ";
   } else {
   #    return " <NPP english=\"GB_MARKUP_ERROR\" prob=\"1.0\"> $pre_stuff $orig_num $post_stuff </NPP> ";
      return " $pre_stuff $orig_num $post_stuff ";
   }
}

sub tran_date {
   my ( $date ) = @_;
   my %date_tran_fun = (
         $date_111 => \&yy_mm_dd,
         $date_110 => \&yy_mm,
         $date_011 => \&mm_dd,
         $date_001 => \&dd,
         $date_010 => \&mm,
         $date_100 => \&yy,
         $date_100_dai => \&yyd,
         );
   unless ( $date =~m/<DATE value=\"([\w:]+)\|(.*)\">(.*)<\/DATE>/ ) {
      die "error in this date expression!";
   }

   my $field1 = $1;
   my $field2 = $2;
   my $field3 = $3;

   my @date_num = split( /:/, $field1 );
   my $date_standard = $field2;

   if ( ! exists $date_tran_fun{ $date_standard } ) {
   #    print STDERR "Invalid date code '$date_standard' in date '$date' ",
   #                         "at line $.\n";
   #    return " <NPPP english=\"GB_MARKUP_ERROR\" prob=\"1.0\">$field3</NPPP> ";
      return " $field3 ";
   }

   my $en_date = &{ $date_tran_fun{ $date_standard } }( @date_num );
   if ( ! $en_date ) {
   #    print STDERR "Empty English date for $date on line $.\n";
      $en_date = "GB_MARKUP_ERROR";
   }
   if ( !defined $field3 ) { $field3 = ''; }
   if ( !defined $en_date ) { $en_date = ''; }

   if ( !( $en_date =~ /GB_MARKUP_ERROR/ ) ) {
      return " <NPPP english=\"" . $en_date . "\" prob=\"1.0\">$field3</NPPP> ";
   } else {
      return " $field3 ";
   }
}

sub yy_mm_dd {
   my @date_num = @_;
   $date_num[ 1 ] += 0;
   my $expression = $gb_en_month2{ $date_num[ 1 ] }
      . " " . $date_num[ 2 ] . " , " . $date_num[ 0 ];
   return $expression;
}

sub yy_mm {
   my @date_num = @_;
   $date_num[ 1 ] += 0;
   my $expression;
   if ( $date_num[ 1 ] > 12 || $date_num[ 1 ] < 1 ) {
      $expression = "[[MARKUP ERROR]] " . $date_num[ 1 ] . " " . $date_num[ 0 ];
   } else {
      $expression = $gb_en_month2{ $date_num[ 1 ] } . " " . $date_num[ 0 ];
   }
   return $expression;
}

sub mm_dd {
   my @date_num = @_;
   $date_num[ 1 ] += 0;
   if ( !defined $date_num[ 2 ] ) { $date_num[ 2 ] = ''; }
   if ( !defined $gb_en_month2{ $date_num[ 1 ] } ) { return '0'; }
   my $expression = $gb_en_month2{ $date_num[ 1 ] } . " " . $date_num[ 2 ];
   return $expression;
}

sub mm {
   my @date_num = @_;
   $date_num[ 1 ] += 0;
   my $expression = $gb_en_month2{ $date_num[ 1 ] };
   return $expression;
}

sub dd {
   my @date_num = @_;
   $date_num[ 2 ] += 0;
   my $expression = $gb_en_day{ $date_num[ 2 ] };
   return $expression;
}

sub yy {
   my @date_num = @_;
   my $expression = $date_num[ 0 ];
   return $expression;
}

sub yyd {
   my @date_num = @_;
   my $expression = $date_num[ 0 ] . 's';
   return $expression;
}

sub commify {
   my $text = reverse @_;
   $text =~ s/(\d\d\d)(?=\d)(?!\d*\.)/$1,/g;
   return scalar reverse $text;
}

sub convert_comma {
   my ( $comma_num ) = @_;
   my ( $temp_num ) = $comma_num;
   $temp_num =~ s/\,//g;
      if ( $temp_num eq '' ) { $temp_num = 0; }
   if ( $temp_num <= 2100 ) {
      return $temp_num;
   } elsif ( $temp_num < 1000000 ) {
      return $comma_num;
   }
   my @count_comma = ();
   if ( $comma_num =~ s/\,/\./ ) {
      push( @count_comma, 'S' );
   }
   while ( $comma_num =~ m/\,/ ) {
      $comma_num =~ s/\,//;
         push( @count_comma, 'S' );
   }
   $comma_num =~ s/0*$//;
   $comma_num =~ s/\.$//;
   if ( $#count_comma >= 6 ) {
      $comma_num .= " [[MARKUP error]]";
   } else {
      $comma_num .= " $SSS{$#count_comma+1}";
   }
   return $comma_num;
}

sub rules {
   my ( $content ) = @_;

   $content =~ s/$date_mark_up/&tran_date($&)/eg;
   $content =~ s/($percent_f[ ]*$num_mark_up)|($num_mark_up[ ]*($percent_b))|($num_mark_up[ ]*$us_dollar)/&tran_num($&)/eg;
   $content =~ s/$num_mark_up[ ]*($more_wan_yi)/&tran_num($&)/eg;
   $content =~ s/$num_mark_up/&tran_num($&)/eg;
   $content =~ s/  +/ /g;
   $content =~ s/^ //;
   $content =~ s/ $//;
   return $content;
}

#==================== mainline ====================

zopen(*IN, "< $in") or die "Can't open $in for reading: $!\n";
zopen(*OUT, "> $out") or die "Can't open $out for writing: $!\n";

binmode( IN,  ":encoding(UTF-8)" );
binmode( OUT, ":encoding(UTF-8)" );

while ( <IN> ) {
   chomp;
   my $content = $_;

   $content = rules( $content );
   $content =~ s/[&]lt;/\\</g;
   $content =~ s/[&]gt;/\\>/g;
   $content =~ s/[&]quot;/\"/g;
   $content =~ s/[&]apos;/\'/g;
   $content =~ s/[&][#]92;/\\\\/g;
   $content =~ s/[&]amp;/\&/g;

# JHJ Apr 5 2007: Allow through all NE translations that came from
#                 previous steps.
   $content =~ s/\\<(NPP[^>]*)\\>/<$1>/g;
   $content =~ s/\\<(\/NPP[^>]*)\\>/<$1>/g;

   print OUT $content, "\n";
}
