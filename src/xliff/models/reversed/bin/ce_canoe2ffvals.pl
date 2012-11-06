#!/usr/bin/perl -sw

use strict;
use warnings;

# command-line
our ($h, $help, $verbose, $debug, $dir, $prefix);

$dir = "." unless defined $dir;
$prefix = "" unless defined $prefix;

my $infile = shift || "-";

open(my $in, "< $infile") or die "Can't open input file $infile";
my $p_text_file = "${dir}/${prefix}p.dec";
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
    chomp $line;

    my $l = scalar(split(/\s+/, $line));
    print {$p_text} $line, "\n";
    print {$p_plen} $l, "\n";
    print {$p_pmax} $l, "\n";
    print {$p_oov} $line, "\n";
    print {$p_score} $., "\n";
    print {$p_ffvals} "1 2 3", "\n";
}

close $in;
close $p_text;
close $p_oov;
close $p_plen;
close $p_pmax;
close $p_score;
close $p_ffvals;

exit 0;

