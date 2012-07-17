#!/usr/bin/env perl
# $Id$

# @file chinese_recode.pl
# @brief Recode frequent bad characters flagged by utf8_filter to something usable.
#
# @author Howard Johnson
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada


use strict;
use utf8;
use Encode;
use Encode::HanConvert;
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
printCopyright "chinese_recode.pl", 2008;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [IN [OUT]]

  Remap characters to unicode code points.

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

my %map_to = (

# map to 'horizontal ellipsis'
  "[[[*85]]]" => "\x{2026}",

# map to 'left single quotation mark'
  "[[[*91]]]" => "\x{2018}",

# map to 'right single quotation mark'
  "[[[*92]]]" => "\x{2019}",

# map to 'left double quotation mark'
  "[[[*93]]]" => "\x{201c}",

# map to 'right double quotation mark'
  "[[[*94]]]" => "\x{201d}",

# map to 'hyphen'
  "[[[*96]]]" => "-",
  "[[[*97]]]" => "-",

# map to space
  "[[[2U+0009]]]" => " ",           # horizontal tabulation
  "[[[3U+00a0]]]" => " ",           # no-break space

# map to 'ideographic comma'
  "[[[3U+ff64]]]" => "\x{3001}",    # halfwidth ideographic comma

# map to 'middle dot'
  "[[[3U+0387]]]" => "\x{00b7}",    # greek ano teleia
  "[[[3U+2022]]]" => "\x{00b7}",    # bullet
  "[[[3U+30fb]]]" => "\x{00b7}",    # katakana middle dot
  "[[[2U+e584]]]" => "\x{00b7}",    #
  "[[[3U+ff65]]]" => "\x{00b7}",    # halfwidth katakana middle dot

# map to zhong1 character
  "[[[3U+3197]]]" => "\x{4e2d}",    # ideographic annotation middle mark

# map to run2 character
  "[[[3U+319f]]]" => "\x{4eba}",    # ideographic annotation man mark

# map to 'tilde'
  "[[[3U+223c]]]" => "~",           # tilde operator

# map to 'greater-than sign'
  "[[[3U+203a]]]" => ">",           # single right-pointing angle quotation mark

# map to 'f' 'l'
  "[[[3U+fb02]]]" => "fl",          # latin small ligature fl

"[[[3U+00c3]]][[[2U+0082]]][[[3U+00c2]]] "             => "\x{201c}",
"[[[3U+00c3]]][[[2U+0082]]][[[3U+00c2]]]¨"             => "\x{201d}",
"[[[3U+00c3]]][[[2U+0082]]][[[3U+00c2]]]·"             => "\x{00b7}",
"[[[3U+00c3]]][[[2U+0082]]][[[3U+00c2]]][[[3U+00a0]]]" => " ",

  "[[[3U+3701]]]" => "\"W",         # kludge

);

my %tabulate;

zopen(*IN, "< $in") or die "Can't open $in for reading: $!\n";
zopen(*OUT, "> $out") or die "Can't open $out for writing: $!\n";

binmode( IN,  ":encoding(UTF-8)" );
binmode( OUT, ":encoding(UTF-8)" );

while ( <IN> ) {
   s/[\r]?[\n]?$//;
   my $oline = $_;
   my $sline = trad_to_simp( $oline );

   my $in_text = $sline;
   my $out_text = '';
   while ( $in_text ) {
      if ( $in_text =~
            s/^(\[\[\[3U[+]00c3\]\]\])(\[\[\[[^\]]*\]\]\]|.)(\[\[\[3U[+]00c2\]\]\])(\[\[\[[^\]]*\]\]\]|[^\[])//
         ) {
         my $ch = $1 . $2 . $3 . $4;
         $out_text .= cnvt1( $ch );
      }
      elsif ( $in_text =~ s/^(\[\[\[[^\]]*\]\]\])// ) {
         my $ch = $1;
      $out_text .= cnvt1( $ch );
   }
   elsif ( $in_text =~ s/^([a-zA-Z0-9 ]+)// ) {
      my $ch = $1;
   $out_text .= $ch;
   }
   elsif ( $in_text =~ s/^(.)// ) {
      my $ch = $1;
   $out_text .= cnvt2( $ch, "" );
   }
   }
   my $pline = $out_text;

   $pline =~ s/ +/ /g;
   $pline =~ s/^ *//;
   $pline =~ s/ *$//;
   print OUT $pline, "\n";
}

close(IN);
close(OUT);

sub cnvt1 {
   my ( $ch ) = @_;
   my $result = $map_to{ $ch };
   if ( $result ) {
      return $result;
   }
   if ( $ch =~ m/^...3U.00c3......2U.0083......3U.00c2......3U.00(..)...$/ ) {
      $map_to{ $ch } = chr( hex( $1 ) + 0x40 );
   }
   elsif ( $ch =~ m/^...3U.00c3......2U.0082......3U.00c2......3U.00(..)...$/ ) {
      $map_to{ $ch } = chr( hex( $1 ) );
   }
   elsif ( $ch =~ m/^\[\[\[3U[+]0([0-3]..)\]\]\]$/ ) {
      $map_to{ $ch } = chr( hex( $1 ) );
   }
   elsif ( $ch =~ m/^\[\[\[[*]([a-f].)\]\]\]$/ ) {
      $map_to{ $ch } = chr( hex( $1 ) );
   }
   $result = $map_to{ $ch };
   if ( $result ) {
      return $result;
   }
   $tabulate{ $ch }++;
   return $ch;
}

sub cnvt2 {
   my ( $ch, $nm ) = @_;
   my $ord = ord $ch;
   if ( $ord >= 0xff01 && $ord <= 0xff5e ) {
      return chr( $ord - 0xff00 + 0x20 );
   }
   if ( $ord == 0x3000 ) {
      return ' ';
   }
   if ( $nm eq 'en.al' ) {
      if ( $ord > 0x3ff ) {
         my $r2 = decode( "iso-8859-1", encode( "big5-hkscs", $ch ) );
         $r2 =~ s/\x{0093}/"/g;
         my $r1 = sprintf( "[[[4U+%4x]]]", $ord );
         my $result = $r2;
         $tabulate{ $result }++;
         return $result;
      }
   }
   return $ch;
}
