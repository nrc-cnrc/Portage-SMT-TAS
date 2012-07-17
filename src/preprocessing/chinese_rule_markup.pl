#!/usr/bin/env perl

# $Id$
#
# @file        chinese_rule_markup.pl
# @brief       Mark up date and number in pseudo-XML format in Chinese text

# @author      Howard Johnson
#
# Note:        This script replaces gb-markup.pl.
#
# Copyright (c) 2006, Conseil national de recherches Canada / National
# Research Council Canada
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html
#
# Usage: gb-markup.pl [-mainland] < in > out
#        With the -mainland option, 兆 is interpreted using its modern/mainland
#        interpretation of million, otherwise it is the traditional/Taiwan
#        interpretation of trillion.

use strict;
use warnings;
require 5.8.5;
use utf8;

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
printCopyright "chinese_rule_markup.pl", 2006;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [IN [OUT]]

  Markup date and number in XML format in Chinese text.

Options:

  -mainland     With the -mainland option, 兆 is interpreted using its
                modern/mainland interpretation of million, otherwise it is the
                traditional/Taiwan interpretation of trillion.

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
   mainland    => \my $mainland,
) or usage;

my $in  = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";



#==========
# The strategy is to construct a complex regular expression
# recursively.  First of all we need the atoms: the characters
# that can occur in numbers.
# int matches a sequence of digits with some embedded blanks.
# dec matches a sequence of digits without embedded blanks
# followed by a decimal marker and a sequence of cd_digits
# with a number of embedded blanks.
#
my $digit
  = "[○零一二两三四五六七八九十百千万亿兆０１２３４５６７８９0123456789多]";
my $int = "(${digit}+([ ]+${digit}+)*)";
my $dec = "(${int}[ ]*[点．·\.][ ]*${int})";
my $num = "(${dec}|${int})";
my $percent = "(百分之[ ]*${num}|${num}[ ]*(%|％))";
my $usd = "(${num}[ ]*美元)";
my $gnum = "(${percent}|${usd}|${num})";
#
#==========

#==========
# Now we give the atoms that occur in date entities.  These are
# the digits from above plus the characters for year, month, and
# day.
#
my $year  = "年";
my $month = "月";
my $day   = "日";
my $datedigit
  = "[○零一二三四五六七八九十０１２３４５６７８９0123456789]";
#
#==========

#==========
# Dates may contain a subset of year, month, and day information.
# Some combinations are rare (errors?).
#
my $d_001 = "(${datedigit}[ ]*){1,3}${day}";
my $d_010 = "(${datedigit}[ ]*){1,2}${month}";
my $d_100 = "(${datedigit}[ ]*){2,4}${year}";
my $d_111 = "${d_100}[ ]*${d_010}[ ]*${d_001}";
my $d_110 = "${d_100}[ ]*${d_010}";
my $d_011 = "${d_010}[ ]*${d_001}";

my $mall = "${d_111}|${d_110}|${d_011}|${d_100}[代]?|${d_010}|${d_001}|${gnum}";
#


sub map_four_group {
   my ( $cn ) = @_;
   my $d = "[一二三四五六七八九]";
   if ( $cn =~ /^(${d}[千]|[零])(${d}[百]|[零])(${d}[十]|[零])(${d}|[零])$/ ) {
      $cn =~ s/[千百十]//g;
      $cn =~ tr/零一二两三四五六七八九/01223456789/;
      return $cn;
   } else {
      $cn =~ s/[千百十]//g;
      $cn =~ tr/零一二两三四五六七八九/01223456789/;
      return "[[4G:" . $cn . "]]";
   }
}


sub transliterate {
   my ( $cn ) = @_;
   $cn =~ tr/一二两三四五六七八九/1223456789/;
   return $cn;
}


sub map_combine {
   my ( $hi, $fr, $lo, $pow ) = @_;
   $hi = ( $hi ? $hi : '1' );
   my $pow2 = $pow - length( $fr ) - length( $lo );
   if ( $pow2 >= 0 ) {
      return $hi . $fr . ( '0' x $pow2 ) . $lo;
   } else {
      return $hi . $fr . "[[" . -$pow2 . "]]" . $lo;
   }
}


sub map_small_int {
   my ( $cn ) = @_;
   my $d = "[一二两三四五六七八九]";
   if ( $cn =~ /^[0-9]*$/ ) {
      return $cn;
   } elsif ( $cn =~ /^([0-9]+)[千]$/ ) {
      return $1 . '000';
   }
   my $tcn = $cn;
   my $result = 0;
   if ( $tcn =~ s/^[千]// ) {
      $result += 1000;
   } elsif ( $tcn =~ s/^(${d})[千]// ) {
      $result += transliterate( $1 ) * 1000;
   } elsif ( $tcn =~ s/^[0]// ) {
   }
   if ( $tcn =~ s/^[百]// ) {
      $result += 100;
   } elsif ( $tcn =~ s/^(${d})[百]// ) {
      $result += transliterate( $1 ) * 100;
   } elsif ( $tcn =~ s/^[0]// ) {
   }
   if ( $tcn =~ s/^[十]// ) {
      $result += 10;
   } elsif ( $tcn =~ s/^(${d})[十]// ) {
      $result += transliterate( $1 ) * 10;
   } elsif ( $tcn =~ s/^[0]// ) {
   }
   if ( $tcn =~ s/^(${d})// ) {
      $result += transliterate( $1 );
   } elsif ( $tcn =~ s/^[0]// ) {
   }
   if ( $tcn eq '' ) {
      return $result . '';
   } else {
#    return "[[SM:" . $cn . "]]";
# Desperation!!!
      $result = transliterate( $cn );
      my $multiplier = 1;
      while ( $result =~ s/千// ) {
         $multiplier *= 1000
      }
      while ( $result =~ s/百// ) {
         $multiplier *= 100
      }
      while ( $result =~ s/十// ) {
         $multiplier *= 10
      }
      if ( $result eq '' ) {
         return $multiplier;
      } elsif ( $result =~ /^[0-9]+$/ ) {
         return $result * $multiplier;
      } else {
         return "[[SM:" . $cn . "]]";
      }
   }
}


sub map_small_dec {
   my ( $cn ) = @_;
   my $n1 = "[0-9一二两三四五六七八九十百千]*";
   my $n2 = "[0-9一二两三四五六七八九]*";
   if ( $cn =~ /^(${n1})[.](${n2})$/ ) {
      my $hi = map_small_int( $1 );
      my $fr = transliterate( $2 );
      if ( $hi =~ /\[\[/ || $fr =~ /\[\[/ ) {
         return "[[SD:" . $hi . ":" . $fr  . "]]";
      } else {
         return "$hi.$fr";
      }
   }
}


sub map_big_int_wan {
   my ( $cn ) = @_;
   if ( $cn =~ /^(.*)[万](.*)$/ ) {
      my $hi = map_small_int( $1 );
      my $lo = map_small_int( $2 );
      if ( $hi =~ /\[\[/ || $lo =~ /\[\[/ ) {
         return "[[BW:" . $hi . ":" . $lo  . "]]";
      } else {
         return map_combine( $hi, '', $lo, 4 );
      }
   } else {
      return map_small_int( $cn );
   }
}


sub map_big_int_yi {
   my ( $cn ) = @_;
   if ( $cn =~ /^(.*)[亿](.*)$/ ) {
      my $hi = map_big_int_wan( $1 );
      my $lo = map_big_int_wan( $2 );
      if ( $hi =~ /\[\[/ || $lo =~ /\[\[/ ) {
         return "[[BY:" . $hi . ":" . $lo  . "]]";
      } else {
         return map_combine( $hi, '', $lo, 8 );
      }
   } else {
      return map_big_int_wan( $cn );
   }
}


sub map_big_int_zhao {
   my ( $cn ) = @_;
   if ( $cn =~ /^(.*)[兆](.*)$/ ) {
      my $hi = map_big_int_yi( $1 );
      my $lo = map_big_int_yi( $2 );
      if ( $hi =~ /\[\[/ || $lo =~ /\[\[/ ) {
         return "[[BZ:" . $hi . ":" . $lo  . "]]";
      } else {
         return map_combine( $hi, '', $lo, ( $mainland ? 6 : 12 ) );
      }
   } else {
      return map_big_int_yi( $cn );
   }
}


sub map_big_dec {
   my ( $cn ) = @_;
   my $n1 = "[0-9一二两三四五六七八九十百千]*";
   my $n2 = "[0-9一二两三四五六七八九]*";
   my $n3 = "[千万亿兆]*";
   if ( $cn =~ /^(${n1})[.](${n2})(${n3})$/ ) {
      my $hi = map_small_int( $1 );
      my $fr = transliterate( $2 );
      my $lo = map_big_int_zhao( $3 );
      if ( $hi =~ /\[\[/ || $lo =~ /\[\[/ ) {
         return "[[SD:$hi:$fr:$lo]]";
      } else {
         return map_combine( $hi, $fr, '', length( $lo ) - 1 );
      }
   }
}


sub map_int {
   my ( $cn ) = @_;
   if ( $cn =~ /^[0-9]*$/ ) {
      return $cn;
   } else {
      return map_big_int_zhao( $cn );
   }
}


sub map_dec {
   my ( $cn ) = @_;
   if ( $cn =~ /[万亿兆]/ ) {
      return map_big_dec( $cn );
   } else {
      return map_small_dec( $cn );
   }
}


sub map_num {
   my ( $cn ) = @_;
   my $result;
   if ( $cn =~ /[.]/ ) {
      $result = map_dec( $cn );
   } else {
      $result = map_int( $cn );
   }
   if ( $result =~ /\[\[/ ) {
      $result = transliterate( $result );
   }
   return $result;
}


sub gb2arabic {
   my ($gb) = @_;

   $gb =~ s/ //g;
   $gb =~ tr/０１２３４５６７８９/0123456789/;
   if ( $gb =~ /^[0-9]*$/ ) {
      return $gb;
   }
   $gb =~ tr/点．·○零/...00/;
   my $n1 = "[0-9一二两三四五六七八九十百千万亿兆.]+";
   $gb =~ s/${n1}/&map_num($&)/eg;
   my $dg1 = "[12一二]";
   my $dg2 = "[0189一八九]";
   my $dg  = "[0-9一二两三四五六七八九]";
   $gb =~ s/\[\[SM:(${dg1}${dg2}${dg}${dg})\]\]年/&transliterate("$1年")/eg;
   $gb =~ s/\[\[SM:(${dg}${dg})\]\]年/&transliterate("$1年")/eg;

   return $gb;

# ○零一二三四五六七八九十百千万亿兆点．·０１２３４５６７８９
}


sub mark_date {
   my ( $date ) = @_;
   $date =~ s/ //g;
   my $date_pattern = $date;
   $date_pattern =~ s/[0-9]+/xx/g;
   my $y_num = "NULL";
   my $m_num = "NULL";
   my $d_num = "NULL";
   if ( $date =~ /([0-9]+)${year}/ ) {
      $y_num = $1;
   }
   if ( $date =~ /([0-9]+)${month}/ ) {
      $m_num = $1;
   }
   if ( $date =~ /([0-9]+)${day}/ ) {
      $d_num = $1;
   }
   return "$y_num:$m_num:$d_num|$date_pattern";
}


sub one_or_two {
   my ( $arg ) = @_;
   if ( $arg =~ s/^十[ ]*一[ ]*二/十二/ ) {
      return "11 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*二[ ]*三/十三/ ) {
      return "12 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*三[ ]*四/十四/ ) {
      return "13 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*四[ ]*五/十五/ ) {
      return "14 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*五[ ]*六/十六/ ) {
      return "15 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*六[ ]*七/十七/ ) {
      return "16 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*七[ ]*八/十八/ ) {
      return "17 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^十[ ]*八[ ]*九/十九/ ) {
      return "18 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^一[ ]*二[ ]*十/二十/ ) {
      return "10 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^二[ ]*三[ ]*十/三十/ ) {
      return "20 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^三[ ]*四[ ]*十/四十/ ) {
      return "30 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^四[ ]*五[ ]*十/五十/ ) {
      return "40 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^五[ ]*六[ ]*十/六十/ ) {
      return "50 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^六[ ]*七[ ]*十/七十/ ) {
      return "60 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^七[ ]*八[ ]*十/八十/ ) {
      return "70 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^八[ ]*九[ ]*十/九十/ ) {
      return "80 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^一[ ]*二([ ]*[^ 0零○０点．·.一二三四五六七八九])/二$1/ ) {
      return "1 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^二[ ]*三([ ]*[^ 0零○０点．·.一二三四五六七八九])/三$1/ ) {
      return "2 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^三[ ]*四([ ]*[^ 0零○０点．·.一二三四五六七八九])/四$1/ ) {
      return "3 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^四[ ]*五([ ]*[^ 0零○０点．·.一二三四五六七八九])/五$1/ ) {
      return "4 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^五[ ]*六([ ]*[^ 0零○０点．·.一二三四五六七八九])/六$1/ ) {
      return "5 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^六[ ]*七([ ]*[^ 0零○０点．·.一二三四五六七八九])/七$1/ ) {
      return "6 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^七[ ]*八([ ]*[^ 0零○０点．·.一二三四五六七八九])/八$1/ ) {
      return "7 or " . gb2arabic( $arg );
   } elsif ( $arg =~ s/^八[ ]*九([ ]*[^ 0零○０点．·.一二三四五六七八九])/九$1/ ) {
      return "8 or " . gb2arabic( $arg );
   } else {
      gb2arabic( $arg );
   }
}


sub avoid_null {
   my ( $arg ) = @_;
   return ( $arg ne '' ? $arg : '[[]]' );
}


sub percenthelper {
   my ( $arg ) = @_;
   my $numm = $arg;
   my $more_than = '';
   $numm =~ s/^百分之[ ]*//;
   $numm =~ s/[ ]*(%|％)$//;
   if ( $numm =~ s/多// ) {
      $more_than = '> ';;
   }
   return " <NUM value=\"" . avoid_null( $more_than . one_or_two( $numm ) )
   . " %\"> $arg </NUM> ";
}


sub usdhelper {
   my ( $arg ) = @_;
   my $numm = $arg;
   my $more_than = '';
   $numm =~ s/[ ]*美元$//;
   if ( $numm =~ s/多// ) {
      $more_than = '> ';;
   }
   return " <NUM value=\"" . avoid_null( $more_than . one_or_two( $numm ) )
   . " dollars\"> $arg </NUM> ";
}


sub numhelper {
   my ( $arg ) = @_;
   my $more_than = '';
   my $numm = $arg;
   if ( $numm =~ s/多// ) {
      $more_than = '> ';;
   }
   return " <NUM value=\"" . avoid_null( $more_than . one_or_two( $numm ) )
   . "\"> $arg </NUM> ";
}


sub datehelper {
   my ( $arg ) = @_;
   return " <DATE value=\"" . &mark_date(&gb2arabic($arg)) . "\"> $arg </DATE> ";
}


sub helper {
   my ( $arg ) = @_;
   $arg =~ s/^([ \n])//;
   my $beginning = $1;
   $arg =~ s/([ \n])$//;
   my $ending = $1;
   if (
         $arg =~ /^[一多]$/
         || $arg =~ /[ ]*一[ ]*一[ ]*/
         || $arg =~ /[ ]*万[ ]*万[ ]*/
      ) {
      return $beginning . $arg . $ending;
   } elsif ( $arg =~ /^${percent}$/ ) {
      return $beginning . percenthelper( $arg ) . $ending;
   } elsif ( $arg =~ /^${usd}$/ ) {
      return $beginning . usdhelper( $arg ) . $ending;
   } elsif ( $arg =~ /^${dec}|${int}$/ ) {
      return $beginning . numhelper( $arg ) . $ending;
   } else {
      return $beginning . datehelper( $arg ) . $ending;
   }
}


sub markup {
   my ( $content ) = @_;

#  $content =~
#    s/(?<![^ ])($d_111|$d_110|$d_011|$d_100[代]?|$d_001|$dec|$int)(?![^ ])/&helper($&)/eg;

   $content =~ s/^/ /;
   $content =~ s/$/ /;
   $content =~ s/[ \n](${mall})[ \n]/&helper($&)/eg;
   $content =~ s/  +/ /g;
   $content =~ s/^ //;
   $content =~ s/ $//;
   return $content;
}


sub freeze {
   my ( $str ) = @_;
   $str =~ s/ /\001/g;
   $str;
}


sub unfreeze {
   my ( $str ) = @_;
   $str =~ s/\001/ /g;
   $str;
}

#==================== mainline ====================

zopen(*IN, "< $in") or die "Can't open $in for reading: $!\n";
zopen(*OUT, "> $out") or die "Can't open $out for writing: $!\n";

binmode( IN,  ":encoding(UTF-8)" );
binmode( OUT, ":encoding(UTF-8)" );

while ( <IN> ) {
   chomp;
   my $content = $_;

   #$content =~ s/(元[ ]*月)/1 月/g;

   $content =~ s/[&]/&amp;/g;
   $content =~ s/[<]/&lt;/g;
   $content =~ s/[>]/&gt;/g;
   $content =~ s/[\"]/&quot;/g;
   $content =~ s/[\']/&apos;/g;
   $content =~ s/[\\]/&#92;/g;
   
   my $modded = markup( $content );
   $modded = unfreeze( $modded );   
   
   print OUT $modded, "\n";
}
