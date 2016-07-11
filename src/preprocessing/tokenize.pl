#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;

# @file tokenize.pl 
# @brief Tokenize and sentence split latin-1 text.
#
# @author George Foster, with minor modifications by Aaron Tikuisis,
#         Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004-2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004-2009, Her Majesty in Right of Canada


use strict;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils", $bin_path;
   }
}
use portage_utils;
printCopyright("tokenize.pl", 2004);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use LexiTools;

my $HELP = "
Usage: tokenize.pl [-v] [-p] -ss|-noss [-notok] [-lang=l] [in [out]]

  Tokenize and sentence-split text in ISO-8859-1 (iso latin 1).

Options:

-v    Write vertical output, with each token followed by its index relative to
      the start of its paragraph, <sent> markers after sentences, and <para>
      markers after each paragraph.
-p    Print an extra newline after each paragraph (has no effect if -v)
-ss   Perform sentence-splitting.
-noss Don't perform sentence-splitting.
      Note: one of -ss or -noss is now required, because the old default (-ss)
      often caused unexpected behaviour.
-notok Don't tokenize the output. [do tokenize]
-pretok Already tokenized. Don't re-tokenize the input. [do tokenize]
-lang Specify two-letter language code: en or fr [en]
-paraline
      File is in one-paragraph-per-line format [no]

Caveat:

  With -ss, consecutive non-blank lines are considered as a paragraph: newlines
  within the paragraph are removed and sentence splitting is performed.  To
  increase sentence splitting accuracy, try to preserve existing paragraph
  boundaries in your text, separating them with a blank line (i.e., two
  newlines), or using -paraline if your input contains one paragraph per line.

  To preserve existing line breaks, e.g., if your input is already
  one-sentence-per-line, use -noss, otherwise your sentence breaks will be
  modified in ways that are almost certainly undesirable.

";

our ($help, $h, $lang, $v, $p, $ss, $noss, $paraline, $notok, $pretok);

if ($help || $h) {
   print $HELP;
   exit 0;
}
$lang = "en" unless defined $lang;
$v = 0 unless defined $v;
$p = 0 unless defined $p;
$ss = 0 unless defined $ss;
$noss = 0 unless defined $noss;
$notok = 0 unless defined $notok;
$pretok = 0 unless defined $pretok;
$paraline = 0 unless defined $paraline;
 
my $in = shift || "-";
my $out = shift || "-";

my $psep = $p ? "\n\n" : "\n";

zopen(*IN, "<$in") || die "Error: tokenize.pl: Can't open $in for reading";
zopen(*OUT, ">$out") || die "Error: tokenize.pl: Can't open $out for writing";

if ( !$ss && !$noss ) {
   die "Error: tokenize.pl: One of -ss and -noss is now required; the old default (-ss) frequently caused unexpected behaviour, so we disabled it.\n";
}
if ( $notok && $pretok ) {
   die "Error: tokenize.pl: Specify only one of -notok or -pretok.\n";
}
if ( $ss && $noss ) {
   die "Error: tokenize.pl: Specify only one of -ss or -noss.\n";
}
if ( $noss && $notok ) {
   warn "Warning: tokenize.pl: Just copying the input since -noss and -notok are both specified.\n";
}
if ( $noss && $pretok ) {
   warn "Warning: tokenize.pl: Just copying the input since -noss and -pretok are both specified.\n";
}

# Enable immediate flush when piping
select(OUT); $| = 1;

while (1)
{
   my $para;
   if ($noss || ($paraline && $p)) {
      unless (defined($para = <IN>)) {
         last;
      }
   } else {
      unless ($para = get_para(\*IN, $paraline)) {
         last;
      }
   }

   my @token_positions = tokenize($para, $lang, $pretok);
   my @sent_positions = split_sentences($para, @token_positions) unless ($noss);

   if ($notok || $pretok) {
      if ($noss) {
         # A bit weird, but the user asked to neither split nor tokenize.
         print OUT $para;
      }
      else {
         # User asked for sentence splitting only, no tokenization.
         my $sentence_start = 0;
         for (my $i = 0; $i < $#sent_positions+1; ++$i) {
            # sent_position indicate the beginning of the next sentence, since
            # we want index to be the end of the sentence, we need the previous
            # tuple's index.
            my $index = $sent_positions[$i]-2;

            my $sentence_end = $token_positions[$index] + $token_positions[$index+1];
            my $sentence = get_sentence($para, $sentence_start, $sentence_end);
            $sentence =~ s/\s*\n\s*/ /g; # remove sentence-internal newlines
            print OUT $sentence;
            print OUT " $sentence_start,$sentence_end" if ($v);
            print OUT ($v ? "<sent>" : "");
            print OUT "\n" unless ($i == $#sent_positions);
            $sentence_start = $token_positions[$sent_positions[$i]];
         }
         print OUT ($v ?  "<para>\n" : $psep);
      }
   }
   else {
      for (my $i = 0; $i < $#token_positions; $i += 2) {
         if (!$noss && $i == $sent_positions[0]) {
            print OUT ($v ? "<sent>\n" : "\n");
            shift @sent_positions;
         }

         print OUT get_collapse_token($para, $i, @token_positions, $notok || $pretok), " ";

         if ($v) {
            print OUT "$token_positions[$i],$token_positions[$i+1]\n";
         }
         print OUT $psep if ($noss && $i < $#token_positions - 2 && substr($para, $token_positions[$i], $token_positions[$i+2] - $token_positions[$i]) =~ /\n/);
      }
      print OUT ($v ?  "<para>\n" : $psep);
   }
}

