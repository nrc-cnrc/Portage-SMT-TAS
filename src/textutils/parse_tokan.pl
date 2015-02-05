#!/usr/bin/env perl
# @file parse_tokan.pl
# @brief Given MADA's database output, extract different form for each words.
#
# @author Samuel Larkin based on Marine Carpuat's work.
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada


use strict;
use warnings;
# DON'T use utf8 since the input is Buckwalter.
#use utf8;

use ULexiTools;

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
printCopyright "parse_tokan.pl", 2014;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
    extract one field from the Tokan 1- or 6-tiered format
    in addition, remove markers for latin words and Arabic words that could not be buckwalter analyzed (?)

    parse_tokan.pl format field < in > out

    where
    - format is 1 or 6 (number of tiers in the Tokan output)
    - field is the 0-based field ID to extract
    - in is a file in the 6-tiered format that Tokan outputs
    - out is the output file

   Options:
     -o(ov)       Extract oovs only from a 6 tiers's field 1.
     -nolc        Do not perform lowercasing on latin words.
     -skipTokenization  Do not perform tokenization on latin words.

";
   exit 1;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
GetOptions(
   oov       => \my $oov,
   nolc      => \my $nolc,
   skipTokenization => \my $skipTokenization,
) or usage;

my $tiers = shift or die "You must provide a tiers!";
my $field = shift or die "You must provide a field!";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

die "-oov must have tiers=6 and field<3" if ($oov and ($tiers != 1 and ($field < 0 or $field > 2)));

# Make ULexitools arguments explicit by giving them variable names.
my $notok  = 0;
my $pretok = 0;
my $xtags  = 0;
setTokenizationLang("en");

#binmode( STDIN,  ":encoding(UTF-8)" );
#binmode( STDOUT, ":encoding(UTF-8)" );

my $sep="·";
while(<STDIN>) {
   my @in = split(/\s+/);
    # remove the <\/?non-MSA> tags which are part of a token, or the entire
    # token if there is no content over than the tag
   my @inclean = ();
   foreach my $i (@in) {
      $i =~ s/\<non-MSA\>//g;
      $i =~ s/\<\/non-MSA\>//g;
      if ( $i !~ m/^\@\@LAT$sep/) {
         push(@inclean, $i);
      }
   }

    # escape the middle dots that are not used as separators.But occur within
    # words that have not been analyzed
   my @out = ();
   foreach my $i (@inclean) {
      my @fields = ($i);
      if ($tiers == 6) {
         @fields = split(/$sep/, $i);
         if (@fields > 6) {
            my @clean = split(/\@\@$sep\@\@/, $i);
            $i = join("\@\@$sep\@\@", @clean);
            @fields = @clean;
            if (@fields != 6) {
               die "Incorrect format: $i should have 6 fields separated by $sep, but it has ", $#fields+1, " \n";
            }
         }
         elsif(@fields < 6) {
            die "Expecting 6 fields in input:$i\n";
         }
      }
        if ($oov) {
           my $normed = &normalize($fields[$field]);
           push(@out, $normed) if (defined($normed));
        }
        else {
           push(@out,&normalize($fields[$field]));
        }
   }
    print join(" ",@out),"\n";
}

sub normalize {
   my ($in) = @_;
   my $out = $in;
   if ($out =~ s/\@\@LAT\@\@//g) {
      unless ($skipTokenization) {
      my $para = $out;
      $out = '';
      my @token_positions = tokenize($para, $pretok, $xtags);
      for (my $i = 0; $i < $#token_positions; $i += 2) {
         $out .= " " if ($i > 0);
         $out .= get_collapse_token($para, $i, @token_positions, $notok || $pretok);
      }
      chomp($out);
   }
      $out = lc($out) unless($nolc);
   }
   elsif ($oov) {
      return undef;
   }

   $out =~ s/\@\@//g;
   return $out;
}

sub normalize2 {
   my ($in )= @_;
   my $out = $in;
   if ($out =~ s/\@\@LAT\@\@//g) {
      $out=`echo "$out" | utokenize.pl --lang-en -noss | utf8_casemap -c l`;
      chomp($out);
   }
   $out =~ s/\@\@//g;
   return $out;
}

