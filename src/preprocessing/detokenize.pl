#!/usr/bin/perl -sw

# @file detokenize.pl 
# @brief Transform tokenized English back to normal English text, with some
# support of French text too.
#
# @author SongQiang Fang and George Foster.
#              Improved handling of French by Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004-2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004-2010, Her Majesty in Right of Canada



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
printCopyright("detokenize.pl", 2004);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP = "
Usage: detokenize.pl [-lang=L] [INPUT] [OUTPUT]

Detokenize tokenized text encoded in iso-8859-1 or Windows cp-1252.

Warning: ASCII quotes are handled assuming there is only one level of quotation.

Options:

-lang=L  Specify two-letter language code: en or fr [en]
         Works well for English, not bad for French.
-latin1  Replace cp-1252 characters by their closest iso-8859-1 equilvalents

";

my $in=shift || "-";
my $out=shift || "-";

our ($help, $h, $lang, $latin1);
$lang = "en" unless defined $lang;
if ($help || $h) {
   print $HELP;
   exit 0;
}

my $apos = qr/['\x92\xB4]/;

open IN, "<$in" or die "Error: Cannot open $in for reading";
open OUT,">$out" or die "Error: Cannot open $out for writing";
my $space=" ";
my ( $word_pre, $word_before , $word_after);
my @double_quote=();
my @single_quote=();
my @out_sentence;
while(<IN>)
{
   chomp $_;
   my @tokens = split /[ ]+/;
   @out_sentence =();#initialize the containers
   @double_quote =();# assume  a pair of quotations only bound to one line of sentence.
   @single_quote =();# it's because in the machine output, quotation without paired is normal.
   # this assumption could be taken off if the text file was grammartically correct.
   while( defined (my $word_pre=shift @tokens) )
   {
      if ($word_pre eq "..") {$word_pre = "...";}

      if( $#out_sentence == -1 ){ # first word just push in
         push ( @out_sentence, $word_pre);
         if( is_quote( $word_pre )){
            check_quote( $word_pre);
         }
      } else {  # if it is not first word, considering the situations(punctuations, brackets, quotes, normal words)
         $word_before= $out_sentence[-1];
         if( is_punctuation($word_pre) ){ # don't add space before the word
            push ( @out_sentence, $word_pre);
         }
         elsif( is_quote( $word_pre) ){ # process quote according it is start or end
            process_quote($word_pre, $word_before);
         }
         elsif( is_bracket( $word_pre)){ # process bracket according it is start or end
            process_bracket($word_pre, $word_before);
         }
         elsif (is_poss($word_pre)) {
            process_poss( $word_pre, $word_before);
         }
         elsif (is_fr_hyph_ending($word_pre)) {
            push ( @out_sentence, $word_pre);
         }
         else{
            process_word( $word_pre, $word_before);
         }
      }

   }
   if ( $latin1 ) {
      foreach (@out_sentence) {
         s/\x80/Euro/g;
         s/\x85/.../g;
         s/\x89/%0/g;
         s/\x8C/OE/g;
         s/\x97/--/g;
         s/\x99/TM/g;
         s/\x9C/oe/g;
         tr/\x82\x83\x84\x86\x87\x88\x8A\x8B\x8E/,f"**^S<Z/;
         tr/\x91\x92\x93\x94\x95\x96\x98\x9A\x9B\x9E\x9F/''""\xB7\-~s>zY/;
      }
   }
   print OUT  @out_sentence,"\n";
   $#out_sentence=-1;
}

sub process_word #ch1, ch2
{
   my( $ch_pre, $ch_before)= @_;
   if( ($ch_pre eq "%") ){ # take care of (%)
      if ( $lang eq "fr" ) {
         push ( @out_sentence, $space );
      }
      push ( @out_sentence, $ch_pre);
   }
   elsif( is_en_price($ch_pre, $ch_before)) {
      push ( @out_sentence, $ch_pre);
   }
   elsif( is_punctuation($ch_before) || is_right_bracket($ch_before)){
      push ( @out_sentence, $space);
      push ( @out_sentence, $ch_pre);
   }
   elsif( is_left_bracket($ch_before)){
      push ( @out_sentence, $ch_pre);
   }
   elsif( is_quote($ch_before)){
      process_quote_before($ch_pre,$ch_before);
   }
   elsif (is_prefix($ch_before)) {
      push ( @out_sentence, $ch_pre);
   }
   else{
      push ( @out_sentence, $space);
      push ( @out_sentence, $ch_pre);
   }
}

sub process_bracket #ch1, ch2
{

   my $ch_pre=shift;
   my $ch_before=shift;
   if( is_right_bracket($ch_pre)){
      push ( @out_sentence, $ch_pre);
   }
   else{
#     if( is_punctuation($ch_before)){
#        push ( @out_sentence, $ch_pre);
#     }
      if( is_quote($ch_before)){
         process_quote_before($ch_pre,$ch_before);
      }
      else{
         push ( @out_sentence, $space);
         push ( @out_sentence, $ch_pre);
      }
   }
}

sub process_quote_before # ch1
{
   my $ch_pre= shift;
   my $ch_before= shift;
   if ( is_double_quote($ch_before)){
      if(&double_quote_not_empty){
         push ( @out_sentence, $ch_pre);
      }
      else{
         push ( @out_sentence, $space);
         push ( @out_sentence, $ch_pre);
      }
   }
   elsif ( is_single_quote($ch_before)){
      if(&single_quote_not_empty){
         push ( @out_sentence, $ch_pre);
      }
      else{
         push ( @out_sentence, $space);
         push ( @out_sentence, $ch_pre);
      }
   }
}

sub process_quote #ch1 ,ch2
{
   my $ch_pre=shift;
   my $ch_before=shift;
   if ( is_double_quote($ch_pre)){# in end place, just push in
      if( &double_quote_not_empty ){
         push ( @out_sentence, $ch_pre);
         pop @double_quote;
      }
      else{# in start place, push a space first (changed on Dec 13 2004)
         push (@double_quote, $ch_pre);
         push ( @out_sentence, $space);
         push ( @out_sentence, $ch_pre);
      }
#     else{# in start place, push a space first if the word before it is not special ch(punctuation,bracket)
#
#        push (@double_quote, $ch_pre);
#        if( is_special( $ch_before)){
#           push ( @out_sentence, $ch_pre);
#        }
#        else{
#           push ( @out_sentence, $space);
#           push ( @out_sentence, $ch_pre);
#        }
#     }
   }
   elsif( is_single_quote($ch_pre)){
      if( $ch_before=~/s$/){# in the situations like ( someones ' something). It is not true always, but mostly.
         push ( @out_sentence, $ch_pre);
      }
      else{
         if( &single_quote_not_empty){
            push ( @out_sentence, $ch_pre);
            pop @single_quote;
         }
         else{# in start place, push a space first (changed on Dec 13 2004)
            push (@single_quote, $ch_pre);
            push ( @out_sentence, $space);
            push ( @out_sentence, $ch_pre);
         }
#        else{
#           push (@single_quote, $ch_pre);
#           if( is_special( $ch_before)){
#              push ( @out_sentence, $ch_pre);
#           }
#           else{
#              push ( @out_sentence, $space);
#              push ( @out_sentence, $ch_pre);
#           }
#        }
      }
   }
}
sub check_quote #$ch
{
   my $ch_pre=shift;
   if ( is_double_quote( $ch_pre )){
      if( &double_quote_not_empty){
         pop @double_quote;
      }
      else{
         push (@double_quote, $ch_pre);
      }
   }
   elsif( is_single_quote($ch_pre)){
      if( &single_quote_not_empty ){
         pop @single_quote;
      }
      else{
         push (@single_quote, $ch_pre);
      }
   }
}
sub is_quote # ch
{
   my $ch_pre=shift;
   return is_double_quote($ch_pre) || is_single_quote($ch_pre);
}
sub is_double_quote # $ch
{
   my $ch_pre=shift;
   # \xAB and \xBB (French angled double quotes) left out intentionally, since
   # they are not glued to the text in French.
   # \x93 and \x94 (English angled double quotes in CP1252) also left out: we
   # treat them as brackets instead, since they are left/right specific
   return ((defined $ch_pre)&&($ch_pre eq "\""));
}

sub is_single_quote # $ch
{
   my $ch_pre=shift;
   # `, \xB4, \x91 and \x92 (back and forward tick, and English angled single
   # quotes in CP1252) left out: we treat them as brackets instead, since they
   # are left/right specific
   return ((defined $ch_pre)&&($ch_pre eq "'"));
}
sub double_quote_not_empty
{
   return ( $#double_quote>= 0);
}

sub single_quote_not_empty
{
   return ( $#single_quote>= 0);
}
sub is_special # $var1
{
   my $ch=shift;
   return (is_bracket($ch) || is_punctuation($ch) );
}
sub is_punctuation # $var1
{
   my $ch_pre=shift;
   return ( $lang eq "fr" ? ($ch_pre =~ m/^(?:[,.!?;\x85]|\.\.\.)$/)
                          : ($ch_pre =~ m/^[,.:!?;]$/));
}
sub is_bracket # $ch
{
   my $ch_pre=shift;
   return ( is_left_bracket($ch_pre) || is_right_bracket($ch_pre) );
}
sub is_left_bracket # $ch
{
   my $ch=shift;
   # Includes left double and single quotes, since they require the same
   # treatment as brackets
   # Excludes < and \x8B since we don't split them in tokenize.pl
   return ( $ch =~ m/^[[({\x91\x93`]$/);
}
sub is_right_bracket #ch
{
   my $ch=shift;
   # Includes right double and single quotes, since they require the same
   # treatment as brackets
   # Excludes > and \x9B since we don't split them in tokenize.pl
   return ( $ch =~ m/^[])}\x92\x94\xB4]$/);
}

sub is_prefix # ch
{
   my $ch=shift;
   return ($lang eq "fr" &&
           $ch =~ /^[cdjlmnst]$apos$|^[a-z]*qu$apos$/oi);
}

# handling plural possessive in English
sub process_poss # ch1, ch2
{
   my $ch_pre=shift;
   my $ch_before=shift;
   if (is_poss($ch_pre)) {
      if ($ch_before =~ /^${apos}s/oi) {
         push ( @out_sentence, substr($ch_pre, 0, 1));
      }
      else {
         push ( @out_sentence, $ch_pre );
      }
   }
}

sub is_poss # ch
{
   my $ch=shift;
   return ($lang eq "en" &&
           $ch =~ /^${apos}s/oi);
}

sub is_fr_hyph_ending #ch
{
   my $ch=shift;
   return ($lang eq "fr" &&
           $ch =~ /^-(?:t-)?(?:je|tu|ils?|elles?|on|nous|vous|moi|toi|lui|eux|en|y|ci|ce|les?|leurs?|la|l[\xC0\xE0]|donc)/oi);
}

sub is_en_price # ch1, ch2
{
   my $ch_pre=shift;
   my $ch_before=shift;
   return ($lang eq "en" && $ch_before eq "\$" && $ch_pre =~ /^\.?\d/oi);
}
