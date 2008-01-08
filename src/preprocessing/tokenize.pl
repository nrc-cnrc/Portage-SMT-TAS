#!/usr/bin/perl -sw

# $Id$
#
# tokenize.pl Tokenize and sent-split text.
# 
# PROGRAMMER: George Foster, with minor modifications by Aaron Tikuisis
#
# Copyright (c) 2004 - 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright (c) 2004 - 2008, Her Majesty in Right of Canada
#
# For further information, please contact :
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html


use strict;
use LexiTools;

print STDERR "tokenize.pl, NRC-CNRC, (c) 2004 - 2008, Her Majesty in Right of Canada\n";

my $HELP = "
Usage: tokenize.pl [-v] [-p] [-noss] [-lang=l] [in [out]]

Tokenize and sentence-split text in ISO-8859-1 (iso latin 1).

Options:

-v    Write vertical output, with each token followed by its index relative to
      the start of its paragraph, <sent> markers after sentences, and <para>
      markers after each paragraph.
-p    Print an extra newline after each paragraph (has no effect if -v)
-noss Don't do sentence-splitting.
-lang Specify two-letter language code: en or fr [en]

";

our ($help, $h, $lang, $v, $p, $noss);

if ($help || $h) {
    print $HELP;
    exit 0;
}
$lang = "en" unless defined $lang;
$v = 0 unless defined $v;
$p = 0 unless defined $p;
$noss = 0 unless defined $noss;
 
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
	unless ($para = get_para(\*IN))
	{
	    last;
	}
    }

   # Replace non-breaking spaces by spaces
   $para =~ s/\xA0/ /g;

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

