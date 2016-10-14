# @file hashtags.pm
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
# perl -MTest::Doctest -e run hashtags.pm


=pod

=encoding utf8

=head1 hashtags.pm

B< >

 This Perl module is intended to contain common utility functions for preparing
 hashtags for translation with Portage.

B< >

=cut


use utf8;

package hashtags;

use strict;
use warnings;

require Exporter;

our (@ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS, $VERSION);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
# symbols to export on request
@EXPORT      = qw(markHashTags hashtagify tokenizeHashtags);
@EXPORT_OK   = qw(testsuite);
%EXPORT_TAGS = ();


# This is a utf8 handling script => io should be in utf8 format
# ref: http://search.cpan.org/~tty/kurila-1.7_0/lib/open.pm
# We need UTF-8 to be able to properly lowercase the input.
use open IO => ':encoding(utf-8)';
use open ':std';  # <= indicates that STDIN and STDOUT are utf8




=pod

# ================ markHashTags =================#

=head1 markHashTags

B< =============================================
 ====== markHashTags                    ======
 =============================================>

=over 4

=item B<DESCRIPTION>

 Transform a hashtag sequence in a xmlish form for Portage.

=item B<SYNOPSIS>

 Convert hashtags to XML markup: "# nice _ hash" --> "<hashtag> nice hash </hashtag>"
 These will be transfered to the translation via Portage's tag transfer mechanism,
 then converted back to standard hashtag format in postprocess_plugin

=back

=head2 markHashTags doctest
 >>> hashtags::markHashTags("# ceci _ est _ un _ hashtag")
 "<hashtag> ceci est un hashtag </hashtag>"

 >>> hashtags::markHashTags("# world")
 "<hashtag> world </hashtag>"

 >>> hashtags::markHashTags(hashtags::tokenizeHashtags("#hello-world"))
 "<hashtag> hello world </hashtag>"

 >>> hashtags::markHashTags(hashtags::tokenizeHashtags("#Tag#Retag"))
 "<hashtag> Tag </hashtag> <hashtag> Retag </hashtag>"

 >>> hashtags::markHashTags(hashtags::tokenizeHashtags("#1hashtag is this"))
 "<hashtag> 1 hashtag </hashtag> is this"

=cut
sub markHashTags($) {
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





=pod

# ================ hashtagify =================#

=head1 hashtagify

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

=back

=head2 hashtagify doctest

 >>> hashtags::hashtagify("<hashtag> ceci est un hashtag </hashtag>")
 "#ceci_est_un_hashtag"

 >>> hashtags::hashtagify("<hashtag>world</hashtag>")
 "#world"

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



# Hyphens: U+2010, U+2011, figure dash (U+2012), n-dash (cp-1252 150, U+2013),
# m-dash (cp-1252 151, U+2014), horizontal bar (U+2015), hyphen (ascii -)
my $wide_dashes = quotemeta("‐‑‒–—―");
my $hyphens = $wide_dashes . quotemeta("-");

# What is a hashtag?
my $hashtag_word_re = qr/#[\p{Word}\-#_]+/;

=pod

=encoding utf8

=head1 tokenizeHashtags

=head2 Usage example

perl -nle 'BEGIN{binmode(STDIN, ":encoding(UTF-8)"); binmode(STDOUT, ":encoding(UTF-8)");} use hashtags; print tokenizeHashtags($_) , "\n"' <<<  "Getting my Oktoberfest on #münchen"

=head2 tokenizeHashtags doctest

 >>> hashtags::tokenizeHashtags("This is #AlloCanada")
 "This is # Allo _ Canada"

 >>> hashtags::tokenizeHashtags("#Olympic2016Canada")
 "# Olympic _ 2016 _ Canada"

 >>> hashtags::tokenizeHashtags("#16")
 "#16"

 >>> hashtags::tokenizeHashtags("#Allo_Canada")
 "# Allo _ Canada"

 >>> hashtags::tokenizeHashtags("#Allo-Canada;")
 "# Allo _ Canada ;"

 >>> hashtags::tokenizeHashtags("#Allo,Canada")
 "# Allo ,Canada"

 >>> hashtags::tokenizeHashtags("<a>#Allo_Canada</a>")
 "<a> # Allo _ Canada </a>"

 >>> hashtags::tokenizeHashtags("#Allo?")
 "# Allo ?"

 >>> hashtags::tokenizeHashtags("#Allo;")
 "# Allo ;"

 >>> hashtags::tokenizeHashtags("#Tag#Retag")
 "# Tag # Retag"

# >>> hashtags::tokenizeHashtags("What is #ашок anyway?")
# "What is # ашок anyway?"

# >>> hashtags::tokenizeHashtags("Working remotely #café")
# "Working remotely # café"

# >>> hashtags::tokenizeHashtags("Getting my Oktoberfest on #münchen")
# "Getting my Oktoberfest on # münchen"

 >>> hashtags::tokenizeHashtags("(#hashtag1 )#hashtag2 [#hashtag3 ]#hashtag4 '#hashtag5'#hashtag6")
 "( # hashtag _ 1 ) # hashtag _ 2 [ # hashtag _ 3 ] # hashtag _ 4 ' # hashtag _ 5 ' # hashtag _ 6"

=cut
sub tokenizeHashtags($) {
   my ($s) = @_;

   sub intern($) {
      my ($s) = @_;
      #print "tokenizeHashtags($s)\n";   # For Debugging.

      # if it's all numeric, it's not a hashtag.
      return $s if ($s =~ m/^#\d+$/);

      # Separate the hash symbol from the word.
      $s =~ s/#/# /;

      # An upper cased letter starts a new word.
      $s =~ s/(\p{Lowercase_Letter})(\p{Uppercase_Letter})/$1_$2/g;

      # Numeric / non-numeric(alpha) boundaries
      $s =~ s/(\d)(\D)/$1_$2/g;
      $s =~ s/(\D)(\d)/$1_$2/g;

      # TODO: if not _ this will split a hashtag.
      $s =~ s{(?:[$hyphens]|_)}{ _ }g;

      return ' ' . $s . ' ';
   }

   # Let's add space in front of the hash symbol.
   $s =~ s/(?<!^)(?<! )#/ #/g;

   #$s =~ s/#([\p{Word}\-#_]+)/' #' . intern($1)/ge;
   $s =~ s/($hashtag_word_re)/intern($1)/ge;

   # Trying to match utf-8 letters and numbers.
   #$s =~ s/#([\p{XPosixAlnum}\-#_]+)/' #' . intern($1)/ge;

   # Normalize spaces.
   $s =~ s{ +}{ }g;
   $s =~ s{^ +}{}g;
   $s =~ s{ +$}{}g;

   return $s;
}


#perl -e 'use hashtags (testsuite); testsuite();'
sub testsuite() {
   require YAML;  # Only at runtime.
   my $tests = YAML::LoadFile('hashtags.yml');
   #use Data::Dumper;
   #print Dumper($tests), "\n";
   my $error_count = 0;
   for my $s (@{$tests->{tests}->{tokenizeHashtags}}) {
      print "hashtags::testsuite::testcase: ", $s->{description}, "\n";
      my $got = tokenizeHashtags($s->{text});
      unless($got eq $s->{expected}) {
         warn "Failed: ", $s->{description},  "\n       Got: ", $got, "\n  Expected: ", $s->{expected};
         $error_count += 1;
      }
   }

   exit $error_count;
}


1;
