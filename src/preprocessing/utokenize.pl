#!/usr/bin/perl -sw

# $Id$
#
# @file utokenize.pl
# @brief Tokenize and sentence split UTF-8 text.
#
# @author George Foster, with minor modifications by Aaron Tikuisis
#             UTF-8 adaptation by Michel Simard.
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004-2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004-2009, Her Majesty in Right of Canada

use utf8;

use strict;
use ULexiTools;
#use locale;
# This is a utf8 handling script => io should be in utf8 format
# ref: http://search.cpan.org/~tty/kurila-1.7_0/lib/open.pm
use open IO => ':encoding(utf-8)';
use open ':std';  # <= indicates that STDIN and STDOUT are utf8

print STDERR "utokenize.pl, NRC-CNRC, (c) 2004 - 2009, Her Majesty in Right of Canada\n";

my $HELP = "
Usage: tokenize.pl [-v] [-p] [-noss] [-lang=l] [in [out]]

  Tokenize and sentence-split text in UTF-8.

Options:

-v    Write vertical output, with each token followed by its index relative to
      the start of its paragraph, <sent> markers after sentences, and <para>
      markers after each paragraph.
-p    Print an extra newline after each paragraph (has no effect if -v)
-noss Don't do sentence-splitting. [do]
-lang Specify two-letter language code: en or fr [en]
-paraline
      File is in one-paragraph-per-line format [no]

Caveat:

  The default behaviour is to split sentences, and the algorithm doing so has
  quadratic cost in the lenght of each paragraph.  To speed up processing and
  increase accuracy, make sure you preserve existing paragraph boundaries in
  your text, separating them with a blank line (i.e., two newlines), or using
  -paraline if your input contains one paragraph per line.

  If your input is already one-sentence-per-line, use -noss, otherwise the
  running time will be quadratic in the length of your file and your sentence
  breaks will be modified in that are almost certainly undesirable.

";

our ($help, $h, $lang, $v, $p, $noss, $paraline);

if ($help || $h) {
    print $HELP;
    exit 0;
}
$lang = "en" unless defined $lang;
$v = 0 unless defined $v;
$p = 0 unless defined $p;
$noss = 0 unless defined $noss;
$paraline = 0 unless defined $paraline;
 
my $in = shift || "-";
my $out = shift || "-";

my $psep = $p ? "\n\n" : "\n";

open(IN, "<$in") || die "Can't open $in for reading";
open(OUT, ">$out") || die "Can't open $out for writing";

# Enable immediate flush when piping
select(OUT); $| = 1;

while (1)
{
    my $para;
    if ($noss)
    {
	unless (defined($para = <IN>))
	{
	    last;
	}
    } else
    {
	unless ($para = get_para(\*IN, $paraline))
	{
	    last;
	}
    }

    my @token_positions = tokenize($para, $lang);
    my @sent_positions = split_sentences($para, @token_positions) unless ($noss);

    for (my $i = 0; $i < $#token_positions; $i += 2) {
	if (!$noss && $i == $sent_positions[0]) {
	    print OUT ($v ? "<sent>\n" : "\n");
	    shift @sent_positions;
	}
	print OUT get_token($para, $i, @token_positions), " ";
	if ($v) {
	    print OUT "$token_positions[$i],$token_positions[$i+1]\n";
	}
	print OUT $psep if ($noss && $i < $#token_positions - 2 && substr($para, $token_positions[$i], $token_positions[$i+2] - $token_positions[$i]) =~ /\n/);
    }
    print OUT ($v ?  "<para>\n" : $psep);
}

