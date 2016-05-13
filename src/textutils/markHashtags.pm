# @file markHashtags.pm
# @brief Transform hashtags to and from a xmlish form for Portage.
#
# @author Samuel Larkin based on Michel Simard's code.
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


# To perform unittest on this Perl module:
# perl -MTest::Doctest -e run markHashtags.pm



=head1 markHashtags.pm

B< >

 This Perl module is intended to contain common utility functions for preparing
 hashtags for translation with Portage.

B< >

=cut



package markHashtags;

require Exporter;
@ISA = qw(Exporter);
# symbols to export on request
@EXPORT = qw(markHashtags, hashtagify);

use strict;
use warnings;



# ================ markHashtags =================#

=head1 SUB

B< =============================================
 ====== markHashtags                    ======
 =============================================>

=over 4

=item B<DESCRIPTION>

 Transform a hashtag sequence in a xmlish form for Portage.

=item B<SYNOPSIS>

 Convert hashtags to XML markup: "# nice _ hash" --> "<hashtag> nice hash </hashtag>"
 These will be transfered to the translation via Portage's tag transfer mechanism,
 then converted back to standard hashtag format in postprocess_plugin

 >>> markHashtags("# ceci _ est _ un _ hashtag")
 "<hashtag> ceci est un hashtag </hashtag>"

 >>> markHashtags("# world")
 "<hashtag> world </hashtag>"

=back

=cut
sub markHashtags($) {
   my ($input) = @_;

   # Yes, that's a finite-state machine with operations on transitions :-)
   my $automaton = {
      start => {
         '#' => { op => \&skipToken, opname=>'skip', state => 'hashtagBegin' },
         '_' => { op => \&outputToken, opname=>'output', state => 'start' },
         'any' => { op => \&outputToken, opname=>'output', state => 'start' } },
      hashtagBegin => {
         '#' => { op => \&skipToken, opname=>'skip', , state => 'hashtagBegin' },
         '_' => { op => \&skipToken, opname=>'skip', , state => 'hashtagContinue' },
         'any' => { op => \&storeToken, opname=>'store', state => 'hashtagEnd' } },
      hashtagContinue => {
         '#' => { op => \&flushHashtag, opname=>'flush', state => 'hashtagBegin' },
         '_' => { op => \&skipToken, opname=>'skip', , state => 'hashtagContinue' },
         'any' => { op => \&storeToken, opname=>'store', state => 'hashtagEnd' } },
      hashtagEnd => {
         '#' => { op => \&flushHashtag, opname=>'flush', , state => 'hashtagBegin' },
         '_' => { op => \&skipToken, opname=>'skip', , state => 'hashtagContinue' },
         'any' => { op => \&flushAndOutput, opname=>'flush+output', state => 'start' } },
   };

   my @output = ();
   my @tag = ();
   my $state = 'start';
   for my $token (split(/\s+/, $input)) {
      my $transition = ($token eq '#' or $token eq '_') ? $token : 'any';
      my $op = $automaton->{$state}->{$transition}->{op};
      my $opname = $automaton->{$state}->{$transition}->{opname};
      $op->($token, \@output, \@tag);
      #print STDERR "[$state($token) = $opname";
      $state = $automaton->{$state}->{$transition}->{state};
      #print STDERR " --> $state]\n";
   }
   flushHashtag('', \@output, \@tag);

   return join(" ", @output);
}

sub outputToken {
   my ($tok, $out, $tag) = @_;
   push @$out, $tok;
}

sub skipToken {
   my ($tok, $out, $tag) = @_;
}

sub storeToken {
   my ($tok, $out, $tag) = @_;

   push @$tag, $tok;
}

sub flushHashtag {
   my ($tok, $out, $tag) = @_;

   push @$out, "<hashtag>", @$tag, "</hashtag>"
      if @$tag;
   @$tag = ();
}

sub flushAndOutput {
   flushHashtag(@_);
   outputToken(@_);
}





# ================ hashtagify =================#

=head1 SUB

B< =============================================
 ====== hashtagify                      ======
 =============================================>

=over 4

=item B<DESCRIPTION>

 Given a hashtag wrapped in <hashtag>...</hashtag>, return its equivalent
 tweeter hashtag form.

=item B<SYNOPSIS>

 This code converts sequences of tokens within <hashtag>...</hashtag>
 into a proper, twitter style hashtag. Initial source-text markup is
 performed in tokenize_plugin, and tags are transfered via Portage's
 tag-transfer mechanism (translate.pl -xtags).

 >>> hashtagify("<hashtag> ceci est un hashtag </hashtag>")
 "#ceci_est_un_hashtag"

 >>> hashtagify("<hashtag>world</hashtag>")
 "#world"

=back

=cut
sub hashtagify($) {
   my ($line) = @_;
   chomp $line;
   while ($line =~ /<hashtag/) { # handle nested hashtags!!
      $line =~ s{<hashtag>([^<]*)</hashtag>}{hashtagify_helper($1)}ge;
   }
   $line =~ s/&gt;/>/g;
   $line =~ s/&lt;/</g;
   $line =~ s/&amp;/&/g;

   return $line;
}

sub hashtagify_helper($) {
    my ($s) = @_;
    
    $s =~ s/^\s+//;
    $s =~ s/\s+$//;
    $s =~ s/[\s\#]+/_/g;
    $s =~ s/^/\#/;
    $s =~ s/\#(\P{Alnum}+)_/$1 \#/; # remove initial punctuation, etc. in hashtags -- could be repeated
    
    return $s;
}
