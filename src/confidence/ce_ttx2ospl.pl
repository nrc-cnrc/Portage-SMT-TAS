#!/usr/bin/perl -s
# @file ce_ttx2ospl.pl 
# @brief Handle TTX files for confidence estimation
# 
# @author Michel Simard
# 
# COMMENTS:
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

=pod 

=head1 SYNOPSIS

ce_ttx2ospl.pl {options} ttx_file

=head1 DESCRIPTION

Convert TTX format file (Trados batch file) to OSPL (one sentence per
line) format.  Priduces output compatible with the confidences
estimation programs (ce_translate.pl, etc.).  Output is written onto
multiple files.  Each file contains as many lines as there are
translation units in the input ttx_file.  All output files are
produced within the directory specified with option -dir (. by
default). Output files are: 

=over 1

=item Q.txt: source language text

=item T.txt: target language text (best match from translation memory)

=item qs.sim: best TM match similarity (percent)

=back

=head1 OPTIONS

=over 1

=item -src=SL         Specify source language [EN-CA]

=item -tgt=TL         Specify target language [FR-CA]

=item -dir=D          Write output files to directory D [D=.]

=item -verbose        Be verbose

=item -debug          Produce debugging info

=item -help,-h        Print this message and exit

=back

=head1 SEE ALSO

ce.pl, ce_train.pl, ce_translate.pl, ce_gen_features.pl, ce_tmx.pl, ce_canoe2ffvals.pl.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 Copyright 2009, Her Majesty in Right of Canada

=cut

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
printCopyright("ce_ttx2ospl.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use strict;
use warnings;
use XML::Twig;
use Time::gmtime;

$|=1;

my $output_layers = ":utf8"; # ':raw:encoding(UTF8):crlf:utf8';

my $DEFAULT_SRC="EN-CA";
my $DEFAULT_TGT="FR-CA";

our ($h, $help, $verbose, $debug, $dir, $q, $t, $sim, $src, $tgt);

if ($h or $help) {
    -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    exit 0;
}

$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;

$dir="." unless defined $dir;
$q = "Q.txt" unless defined $q;
$t = "T.txt" unless defined $t;
$sim = "qs.sim" unless defined $sim;
$src = $DEFAULT_SRC unless defined $src;
$tgt = $DEFAULT_TGT unless defined $tgt;

my $infile = shift or die "Missing argument: infile";

if (not -d $dir) {
    mkdir $dir or die "Can't make directory $dir: errno=$!";
}

open(my $qout, ">${output_layers}", "$dir/$q") or die "Can't open output $dir/$q";
open(my $tout, ">${output_layers}", "$dir/$t") or die "Can't open output $dir/$t";
open(my $simout, ">${output_layers}", "$dir/$sim") or die "Can't open output $dir/$sim";

processTTX(infile=>$infile, qout=>$qout, tout=>$tout, simout=>$simout, src_lang=>$src, tgt_lang=>$tgt);

close $qout;
close $tout;
close $simout;

exit 0;



sub processTTX {
    my %args = ( @_ );

    my $parser = XML::Twig->new( twig_handlers=> { Tu => \&processTU },
                                 keep_encoding=>0, 
                                 output_encoding=>'UTF8');

    @{$parser}{keys %args} = values %args; # Merge args into parser
    $parser->{tu_count} = 0;
    $parser->{seg_count} = 0;

    verbose("[Processing TTX file %s ...]\n", $parser->{infile});
    $parser->parsefile($parser->{infile});

    $parser->purge();
    verbose("\r[%d. Done]\n", $parser->{seg_count});
}

sub processTU {
    my ($parser, $tu) = @_;
    
    my $tuid = $tu->{att}{tuid};
    $tuid = $parser->{tu_count} unless defined $tuid;

    my $match_percent = $tu->{att}->{'MatchPercent'};
    $match_percent = 0 unless defined $match_percent;

    my @tuvs = $tu->children('Tuv');
    warn("Missing variants in TU $tuid") if (!@tuvs);

    my $src_tuv = 0;
    my $tm_tuv = 0;
    foreach my $tuv (@tuvs) {
        my $lang = $tuv->{att}->{'Lang'};
        warn("Missing language attribute in TU $tuid") unless $lang;
        
        if ($lang eq $parser->{src_lang}) {
            warn("Duplicate source-language tuv\n") if $src_tuv;
            $src_tuv = $tuv;
            
        } elsif ($lang eq $parser->{tgt_lang}) {
            warn("Duplicate target-language tuv\n") if $tm_tuv;
            $tm_tuv = $tuv;
        }
    }
    if ($src_tuv) {
        # translate source text
        my $src_seg = harvestText($src_tuv);

        # Get TM text
        my $tm_seg = harvestText($tm_tuv);
        
        $parser->{seg_count}++;
        verbose("\r[%d...]", $parser->{seg_count});

        print {$parser->{qout}} normalizeText($src_seg), "\n";
        print {$parser->{tout}} normalizeText($tm_seg), "\n";
        print {$parser->{simout}} $match_percent+0, "\n"; # convert to numeric
    }
    
    $parser->{tu_count}++;

    $parser->purge();
}

sub harvestText {
    my ($head) = @_;
    my @text = ();

    return "" if not $head;

    my @children = $head->children();
    if (not @children) {
#        debug(" harvestText: no children, text=\"%s\"\n", $head->text());
          push @text, $head->text();
    }
    for my $child (@children) {
        if ($child->name() eq '#PCDATA') {
#            debug(" harvestText: PCDATA, text=\"%s\"\n", $child->text());
            push @text, $child->text();
        } elsif ($child->name() eq 'df') {
#            debug(" harvestText: df -- going recursive\n");
            push @text, harvestText($child);
        } elsif ($child->name() ne 'ut') {
            warn "Unknown element in a Tuv:", $child->name(), "\n";
        }
    }

#    debug(" harvestText: return \"%s\"\n", join(" ", @text));
    return join(" ", @text);
}

sub normalizeText {
    my ($text) = @_;

    $text =~ s/\s+/ /g;
    $text =~ s/[<>]/\\$&/g;
    $text =~ s/^\s*//;
    $text =~ s/\s*$//;

    return $text;
}

sub verbose { printf STDERR @_ if $verbose }
sub debug { printf STDERR @_ if $debug }

exit 0;

