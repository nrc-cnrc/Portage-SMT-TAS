#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;

# @file ce_distance.pl 
# @brief Compute per sentence distance (various metrics)
# 
# @author Michel Simard
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

=pod 

=head1 SYNOPSIS

ce_distance.pl {options} text1 text2

=head1 DESCRIPTION

Compute per-sentence distance, using various metrics.  Default is word-based Levenshtein.  Output goes to standard output.

=head1 OPTIONS

=over 1

=item -metric=M         One of: lev, lcs or nX, where X is a positive integer; also available are src and tgt [lev]

=item -unit=U           One of: word, char [word]

=item -norm=N           One of: non, src, tgt, min, max, sum [non]

=item -verbose          Be verbose

=item -debug            Produce debugging info

=item -help, -h         Print this message and exit

=item -man              Print manual page and exit

=back

=head1 SEE ALSO

CE::distance

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 Copyright 2010, Her Majesty in Right of Canada

=cut

use strict;

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
printCopyright("ce_distance.pl", 2010);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use CE::distance;
use List::Util qw(min max);

our($help, $h, $H, $Help, $man, $verbose, $debug);

if ($help or $h) {
    system "podselect -section SYNOPSIS -section OPTIONS -section COPYRIGHT $0 | pod2text";
    exit 0;
}
if ($man or $Help or $H) {
    -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    exit 0;
}

our($metric, $unit, $norm);

$metric = "lev" unless defined $metric;
$unit = "word" unless defined $unit;
$norm = "non" unless defined $norm;

$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;

my $text1 = shift or die "Error: Missing argument: text1";
my $text2 = shift or die "Error: Missing argument: text2";

warn "**Warning: ignoring extract arguments ".join(' ', @ARGV)."\n" if @ARGV;

open(my $t1, "<$text1") or die "Error: Can't open text1 file $text1";
open(my $t2, "<$text2") or die "Error: Can't open text2 file $text2";

my $splitfun = ($unit eq 'word' ? \&splitWords : 
                $unit eq 'char' ? \&splitChars :
                die "Error: Unsupported unit $unit");

my $dfun;
my $n = 1;
if ($metric eq 'lev') {
    $dfun = \&CE::distance::Levenshtein;
} elsif ($metric eq 'lcs') {
    $dfun = \&CE::distance::longestCommonSubstring;
} elsif ($metric =~ /^n(\d+)$/) {
    $dfun = \&CE::distance::commonNgrams;
    $n = $1;
} elsif ($metric eq 'src') {
    $dfun = \&normSrc;           # Ugly
} elsif ($metric eq 'tgt') {
    $dfun = \&normTgt;           # Ugly
} else {
    die "Error: Unsupported metric $metric";
}

my $normfun = ($norm eq 'non' ? \&normNon : 
               $norm eq 'src' ? \&normSrc : 
               $norm eq 'tgt' ? \&normTgt : 
               $norm eq 'min' ? \&normMin : 
               $norm eq 'max' ? \&normMax : 
               $norm eq 'sum' ? \&normSum : 
               die "Error: Unsupported normalization $norm");

verbose("[Computing $unit-based, $norm-normalized $metric, on $text1 and $text2]\n");

$|=1;
my $cnt=0;
while (my $line1 = <$t1>) {
    verbose("\r[%d lines...]", $cnt) if (++$cnt % 100 == 0);
    defined (my $line2 = readline($t2)) 
        or die "Error: Too many lines in text1 $text1";
    my @s1 = &$splitfun($line1);
    debug("|s1| = %d\n", int(@s1));
    my @s2 = &$splitfun($line2);
    debug("|s2| = %d\n", int(@s2));
    my $distance = &$dfun(\@s1, \@s2, $n);
    my $norm = &$normfun(\@s1, \@s2, $n);
    debug("$distance / $norm = %.6f\n", $distance/$norm);
    printf("%.6g\n", $distance/$norm);
}
die "Error: Too many lines in text2 $text2" unless eof $t2;

verbose("\r[%d lines. Done.]\n", $cnt);

close $t1;
close $t2;

exit 0;

sub splitChars {
    my ($s) = @_;
    chomp $s;
    debug("Calling splitChars($s)\n");
    return split(//, $s);
}

sub splitWords {
    my ($s) = @_;
    chomp $s;
    debug("Calling splitWords($s)\n");
    $s =~ s/^\s+//;
    $s =~ s/\s+$//;
    return split(/\s+/, $s);
}

sub normNon {
    my ($t1, $t2, $n) = @_;
    return 1;
}

sub normSrc {
    my ($t1, $t2, $n) = @_;
    return int(@$t1) + 1 - $n;
}

sub normTgt {
    my ($t1, $t2, $n) = @_;
    return int(@$t2) + 1 - $n;
}

sub normMin {
    my ($t1, $t2, $n) = @_;
    return min(int(@$t1), int(@$t2)) + 1 - $n;
}
sub normMax {
    my ($t1, $t2, $n) = @_;
    return max(int(@$t1), int(@$t2)) + 1 - $n;
}
sub normSum {
    my ($t1, $t2, $n) = @_;
    return normSrc($t1, $t2, $n) + normTgt($t1, $t2, $n);
}
sub verbose { printf STDERR @_ if $verbose; }
sub debug { printf STDERR @_ if $debug; }
