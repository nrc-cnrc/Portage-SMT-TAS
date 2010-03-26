#!/usr/bin/perl -sw

# @file ce_canoe2ffvals.pl 
# @brief Parses canoe output and generates FileFF-style files.
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

ce_canoe2ffvals.pl {options} [ filename ]

=head1 DESCRIPTION

Read canoe output from filename (stdin if not specified), as produced 
by the canoe decoder, and produce output in a format compatible with 
the confidence estimation programs (ce_translate.pl, etc.). It is
assumed that canoe was called with -trace (produces alignment info)
and -ffvals (feature function values). 

Output is written to multiple files.  Each file contains as many
lines as canoe's output file.  All output files are produced within
the directory specified with option -dir (. by default). Output files
are: 

=over 1

=item p.tok: tokenized output text

=item p.oov: per-sentence OOV count

=item p.plen: per-sentence phrase count

=item p.pmax: length of longest target phrase used in translation

=item p.ffvals: individual decoding model feature function values

=item p.score: global decoding score (weighted sum of individual ffvals)

=back

=head1 OPTIONS

=over 1 

=item -dir=D          Write output files to directory D [D=.]

=item -prefix=P       Prefix each output file name with prefix P [none]

=item -verbose        Be verbose

=item -debug          Produce debugging info

=item -help,-h        Print this message and exit

=back

=head1 SEE ALSO

ce.pl, ce_train.pl, ce_translate.pl, ce_tmx.pl, ce_ttx2ospl.pl.

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
printCopyright("ce_canoe2ffvals.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use strict;
use warnings;

# command-line
our ($h, $help, $verbose, $debug, $dir, $prefix);

if (defined($h) or defined($help)) {
    -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    exit 0;
}
$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;
$dir = "." unless defined $dir;
$prefix = "" unless defined $prefix;

my $infile = shift || "-";

open(my $in, "< $infile") or die "Can't open input file $infile";
my $p_text_file = "${dir}/${prefix}p.tok";
my $p_oov_file = "${dir}/${prefix}p.oov";
my $p_plen_file = "${dir}/${prefix}p.plen";
my $p_pmax_file = "${dir}/${prefix}p.pmax";
my $p_score_file = "${dir}/${prefix}p.score";
my $p_ffvals_file = "${dir}/${prefix}p.ffvals";

open(my $p_text, "> ${p_text_file}") or die "Can't open output file ${p_text_file}";
open(my $p_oov, "> ${p_oov_file}") or die "Can't open output file ${p_oov_file}";
open(my $p_plen, "> ${p_plen_file}") or die "Can't open output file ${p_plen_file}";
open(my $p_pmax, "> ${p_pmax_file}") or die "Can't open output file ${p_pmax_file}";
open(my $p_score, "> ${p_score_file}") or die "Can't open output file ${p_score_file}";
open(my $p_ffvals, "> ${p_ffvals_file}") or die "Can't open output file ${p_ffvals_file}";

while (my $line = <$in>) {
    chop $line;
    my %p = parseCanoeOutput($line);

    print {$p_text} $p{text}, "\n";
    print {$p_plen} $p{phrases}, "\n";
    print {$p_pmax} $p{pmax}, "\n";
    print {$p_oov} $p{oov}, "\n";
    print {$p_score} $p{score}, "\n";
    print {$p_ffvals} join("\t", @{$p{ffvals}}), "\n";
}

close $in;
close $p_text;
close $p_oov;
close $p_plen;
close $p_pmax;
close $p_score;
close $p_ffvals;

exit 0;

sub parseCanoeOutput {
    my ($canoe_out) = @_;
    chomp $canoe_out;
    my @phrases = ();
    my $log_score = 0;
    my $oov_count = 0;
    my $pmax = 0;
    my @ffvals = ();
    
    debug("canoe_out=\"%s\"\n", ${canoe_out});    
    while ($canoe_out =~ /\"([^\\\"]*(?:\\[\\\"][^\\\"]*)*)\" a=\[([^\]]+)\]( +v=\[[^\]]+\])? */g) {
        my $phrase = $1;
        my $align = $2;
        my $v = $3;
        my ($score, $from, $to, $oov) = split(/;/, $align);
        push @phrases, $phrase;
        $log_score += log($score);
        ++$oov_count if $oov eq 'O';
        my @words = split(/\s+/, $phrase);
        my $plen = int(@words);
        $pmax = $plen if $plen > $pmax;
        if ($v) {
            die "Unexpected v= format: $v"
                unless ($v =~ /v=\[([^\]]+)\]/);
            my $fv = $1;
            my @fv = split(/;/, $fv);
            if (!@ffvals) {
                @ffvals = @fv;
            } else {
                for (my $i = 0; $i <= $#fv; ++$i) {
                    $ffvals[$i] += $fv[$i];
                }
            }
        }
        debug("phrase(\"%s\", score=%f, oov=%s)\n", $phrases[-1], $score, $oov);
    }
    return (text=>join(" ", @phrases),
            phrases=>int(@phrases),
            pmax=>$pmax,
            score=>$log_score, 
            oov=>$oov_count,
            ffvals=>[ @ffvals ]);
}

sub verbose { printf STDERR @_ if $verbose }
sub debug { printf STDERR @_ if $debug }

