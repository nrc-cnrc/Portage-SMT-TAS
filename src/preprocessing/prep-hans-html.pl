#!/usr/bin/perl -s

# @file prep-hans-html.pl 
# @brief Preprocess Hansard HTML format files.
# 
# @author George Foster
# 
# COMMENTS: 
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use HTML::TokeParser();
use locale;

print STDERR "prep-hans-html.pl, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada\n";

$HELP = "
prep-hans-html.pl [in [out]]

Preprocess an HTML-coded Hansard file.

Options:

-n Omit re-coded markup (source language, date, time, sect) in output.


";

if ($help || $h) {
    print $HELP;
    exit 0;
}

$in = shift || "-";
$out = shift || "-";

if (!open(IN, "<$in")) {die "Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Can't open $out for reading";}

$p = HTML::TokeParser->new(\*IN);

$last_nl = 0;
$last_blank = 0;
$in_preamble = 1;

while ($tok = $p->get_token) {

    if ($tok->[0] eq "T") {	# text token
	$text = $tok->[1];
	
	# general cleanup
	$text =~ s/(&nbsp;)+/ /go;
	$text =~ s/&eacute;/\xE9/go;
	$text =~ s/\x92/\'/go;
 	$text =~ s/\x93/``/go;
 	$text =~ s/\x94/''/go;
 	$text =~ s/\x95|\x97/--/go;
	$text =~ s/\x9C/oe/go;
	$text =~ s/\n(\s*\n)+/\n\n/go;
	$text =~ s/^\s*_+\s*$/\n\n/o;

	if ($text =~ /^\s*(\*\s*){3}$/o) {$in_preamble = 0;}

	if ($in_preamble && ($text eq "CONTENTS" || $text =~ /TABLE DES MATI.RES/o)) {
	    skip_contents();
	    next;
	}
	
	# convert useful markup
	if (!$n) {
	    # eg [<i>English<i>] -> [English]
	    if ($text =~ /^(\n?)\[$/o) {
		$text = $1 . "[" . $p->get_phrase;
	    }
	    $text =~ s/^(\s*)\[(Translation|Fran.ais)\]\s*$/\1<srclang fr>\n/o;
	    $text =~ s/^(\s*)\[(Traduction|English)\]\s*$/\1<srclang en>\n/o;
	    $text =~ s/^D?\s*\(([0-9]{4})\s*\)\s*$/<time \1>\n/o;
	    $text =~ s/^\s*(\*(\s|&nbsp;?)*){3}$/<sect>\n/o;
	    
	    if ($last_blank) {
		$text =~ s/^ ([0-9]{4})\s*$/<time \1>\n/o;
	    }

	}
	# wipe out non-useful markup
	$text =~ s/^\s*\[.+\]\s*$//go;

	# reduce repeated blank lines
	$is_blank_with_nl = ($text =~ /^\s*\n\s*$/o);
	if ($last_blank && $is_blank_with_nl) {next;}
	
	print OUT "$text";

	$last_blank = $last_nl && $is_blank_with_nl || ($text =~ /^\s*\n\s*\n\s*$/o);
	$last_nl = ($text =~ /\n\s*$/o);


    # check for blocks for special processing

    } elsif ($tok->[0] eq "S") {
	if ($tok->[1] =~ /^(style|a|span)$/o) {
	    skip_to_end($tok->[1]);
 	} elsif ($tok->[1] eq "table" && $in_preamble) {
 	    skip_to_end("table");
 	} elsif ($tok->[1] eq "p") {
	    # this big mess tries to strip out hunks of stupid markup that
	    # cause lots of paragraphs to be split in two arbitrarily
 	    my @stupid_seq = ("hr", "b", "a", "a", "text", "a", "a", "b", "p");
	    if (match_tag_seq(@stupid_seq)) {
		$tok = $p->get_token;
		
		if ($tok->[0] eq "T" && $tok->[1] =~ /^\n\w/o) {
		    print OUT substr($tok->[1], 1); # lose the \n
		} else {
		    $p->unget_token(($tok));
		}
	    } else {
		if (!$last_blank) {
		    print OUT ($last_nl ? "\n" : "\n\n");
		    $last_nl = $last_blank = 1;
		}
	    }
	}
    }

# print OUT "<< $tok->[0] $tok->[1] $last_blank>>\n";

}

# For a given tag, skip everything in <tag> ... </tag>, inclusive.  In the
# commented-out recursive call I was trying to cleverly avoid getting caught by
# nested structures, but this turns out to be redundant because the parser
# doesn't even recognize nested start tags. Maybe they're not allowed in
# HTML?
sub skip_to_end {
    my ($tag) = @_; 

    while ($tok = $p->get_token) {
#	if ($tok->[0] eq "S" && $tok->[1] eq $tag) { # wipe nested thingies too
#	    skip_to_end($tag);
#	}
	if ($tok->[0] eq "E" && $tok->[1] eq $tag) {last;}
    }
}

# Match a given sequence of tags exactly (possibly with whitespace between
# tags) - return longest matching prefix of the sequence (with whitespace
# filtered out). The special tag "text" means match any non-white text.
sub match_tag_seq {
    my @tags_to_match = @_;
    my @toks_read;
    foreach $tag (@tags_to_match) {
	my $tok = $p->get_token;
	@toks_read[++$#toks_read] = $tok;

#	print STDERR "{$tag: $tok->[0] $tok->[1]}";

	if ((($tok->[0] eq "S" || $tok->[0] eq "E") && $tok->[1] eq $tag) ||
	    $tok->[0] eq "T" && $tag eq "text")	{
	    ;
	} elsif ($tok->[0] eq "T" && $tok->[1] =~ /^\s+$/o) {
	    unshift (@tags_to_match, ($tag)); # just skip white
	} else {
	    $p->unget_token(@toks_read);
	    return 0;
	}
    }
    return 1;
}

# Skip the whole contents section: everything from the initial CONTENTS block
# to the next blank line.
sub skip_contents {
    my $tok;
    while ($tok = $p->get_token) {
	if ($tok->[0] eq "T" && $tok->[1] =~ /^\s*\n\s*\n$/o) {last;}
    }
}

close IN;
close OUT;
