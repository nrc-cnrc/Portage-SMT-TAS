#!/usr/bin/perl -sw

# $Id$
#
# detokenize.pl - Transform tokenized English back to normal English text, with
#                 minimal support of French text too.
#
# Programmers: SongQiang Fang and George Foster.
#
# Copyright (c) 2004 - 2007, Sa Majeste la Reine du Chef du Canada / Her Majesty in Right of Canada
#
# For further information, please contact :
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html



use strict ;
use LexiTools ;

print STDERR "detokenize.pl, NRC-CNRC, (c) 2004 - 2007, Her Majesty in Right of Canada\n";

my $HELP = "
Usage: detokenize.pl [-lang=l] [input file] [output file]

Detokenize tokenized text.

Warning: Assumes there is only one level of quotation!

Options:

-lang  Specify two-letter language code: en or fr [en]
       Works well for English, but has only minimal support for French.

";

my $in=shift || "-";
my $out=shift || "-";

our ($help, $h, $lang);
$lang = "en" unless defined $lang;
if ($help || $h) {
    print $HELP;
    exit 0;
}

open IN, "<$in" or die " Can not open $in for reading";
open OUT,">$out" or die " Can not open $out for writing";
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
	    else{
		process_word( $word_pre, $word_before);
	    }
	}
	
    }
    print OUT  @out_sentence,"\n";
    $#out_sentence=-1;
}
sub process_word
{
    my( $ch_pre, $ch_before)= @_;
    if( ($ch_pre eq "%") ){# take care of (%)
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
# 	if( is_punctuation($ch_before)){
# 	    push ( @out_sentence, $ch_pre);
# 	}
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
    if ( is_double_quote($ch_pre)){# in end plcae, just push in
				       if( &double_quote_not_empty ){
					   push ( @out_sentence, $ch_pre);
					   pop @double_quote;
				       }
				       else{# in start  palce, push a space first (changed on Dec 13)
						push (@double_quote, $ch_pre);
						push ( @out_sentence, $space);
						push ( @out_sentence, $ch_pre);
					    }
#		else{# in start  palce, push a space first if the word before it is not special ch(punctuation,bracket)
#			
#			push (@double_quote, $ch_pre);
#			if( is_special( $ch_before)){
#			        push ( @out_sentence, $ch_pre);
#			}
#			else{	
#				push ( @out_sentence, $space);
#				push ( @out_sentence, $ch_pre);
#			}
#		}
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
	    else{# in start  palce, push a space first (changed on Dec 13)
		     push (@single_quote, $ch_pre);
		     push ( @out_sentence, $space);
		     push ( @out_sentence, $ch_pre);
		 }
#			else{
#		 		push (@single_quote, $ch_pre);
#				if( is_special( $ch_before)){
#			        	push ( @out_sentence, $ch_pre);
#				}
#				else{	
#					push ( @out_sentence, $space);
#					push ( @out_sentence, $ch_pre);
#				}
#			}
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
    return ((defined $ch_pre)&&($ch_pre eq "\""));
}

sub is_single_quote # $ch
{
    my $ch_pre=shift;
    return ((defined $ch_pre)&&($ch_pre eq "'"));
}
sub quote_not_empty {
    return ( &double_quote_not_empty || &single_quote_not_empty) ;
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
    return ( $ch_pre =~ m/^[,.:!?;]$/);
}
sub is_bracket # $ch
{
    my $ch_pre=shift;
    return ( is_left_bracket($ch_pre) || is_right_bracket($ch_pre) );
}
sub is_left_bracket # $ch
{
    my $ch=shift;
    return ( $ch =~ m/^[\[\(\{<]$/);
}
sub is_right_bracket #ch
{
    my $ch=shift;
    return ( $ch =~ m/^[\]\)\}>]$/);
}

sub is_prefix # ch
{
    my $ch=shift;
    return ($lang eq "fr" &&
	    $ch =~ /^[cdjlmnst]\'|[a-z]*qu\'/oi);
}

# handling plural possessive in English
sub process_poss # ch1, ch2
{
  my $ch_pre=shift;
  my $ch_before=shift;
  if (is_poss($ch_pre)) {
    if (ends_with_s($ch_before)) {
      push ( @out_sentence, "\'");
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
	    $ch =~ /^\'s/oi);
}
sub ends_with_s # ch
{
    my $ch=shift;
    return ($lang eq "en" &&
	    $ch =~ /^.*s/);
}
