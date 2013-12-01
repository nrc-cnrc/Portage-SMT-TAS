#!/usr/bin/env perl
# @file
# @brief
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

die <<USAGE
    extract one field from the Tokan 1- or 6-tiered format
    in addition, remove markers for latin words and Arabic words that could not be buckwalter analyzed (?)

    parse_tokan.pl format field < in > out

    where
    - format is 1 or 6 (number of tiers in the Tokan output)
    - field is the 0-based field ID to extract
    - in is a file in the 6-tiered format that Tokan outputs
    - out is the output file

USAGE
    unless (@ARGV == 2);

my ($tiers, $field) = @ARGV;

my $notok  = 0;
my $pretok = 0;
my $xtags  = 0;
setTokenizationLang("en");

my $sep="·";
while(<STDIN>){
    my @in = split(/\s+/);
    # remove the <\/?non-MSA> tags which are part of a token, or the entire token if there is no content over than the tag
    my @inclean = ();
    foreach my $i (@in){
	$i =~ s/\<non-MSA\>//g;
	$i =~ s/\<\/non-MSA\>//g;
	if ( $i !~ m/^\@\@LAT$sep/){
	    push(@inclean,$i);
	}
    }

    # escape the middle dots that are not used as separators.But occur within words that have not been analyzed
    my @out = ();
    foreach my $i (@inclean){
	my @fields = ($i);
	if ($tiers == 6){
	    @fields = split(/$sep/,$i);
	    if (@fields > 6){
		my @clean = split(/\@\@$sep\@\@/,$i);
		$i = join("\@\@$sep\@\@",@clean);
		@fields = @clean;
		if (@fields != 6) {
		    die "Incorrect format: $i should have 6 fields separated by $sep, but it has ",$#fields+1," \n";
		}
	    }
            elsif(@fields < 6) {
		die "Expecting 6 fields in input:$i\n";
	    }
	}
	push(@out,&normalize($fields[$field]));
    }
    print join(" ",@out),"\n";
}

sub normalize(){
    my ($in )= @_;
    my $out = $in;
    if ($out =~ m/\@\@LAT\@\@/){
	$out =~ s/\@\@LAT\@\@//g;
        my $para = $out;
        $out = '';
        my @token_positions = tokenize($para, $pretok, $xtags);
        for (my $i = 0; $i < $#token_positions; $i += 2) {
           $out .= " " if ($i > 0);
           $out .= get_collapse_token($para, $i, @token_positions, $notok || $pretok);
        }
	chomp($out);
        $out = lc($out);
    }
    $out =~ s/\@\@//g;
    return $out;
}

sub normalize2(){
    my ($in )= @_;
    my $out = $in;
    if ($out =~ m/\@\@LAT\@\@/){
	$out =~ s/\@\@LAT\@\@//g;
	$out=`echo "$out" | utokenize.pl --lang-en -noss | utf8_casemap -c l`;
	chomp($out);
    }
    $out =~ s/\@\@//g;
    return $out;
}

