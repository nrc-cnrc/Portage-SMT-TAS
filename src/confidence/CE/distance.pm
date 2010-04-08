# $Id$
# @file CE::distance.pm
# @brief Text distance metrics for confidence estimation.
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

=head1 NAME

CE::distance 

=head1 SYNOPSIS

 use CE::distance;

 my $s1 = [ qw(a sequence of word tokens) ];
 my $s2 = [ qw(another sequence of tokens) ];
 my $lev = Levenshtein($s1, $s2);
 my $lcs = longestCommonSubstring($s1, $s2);

 my $N = 3;
 my $cng = commonNgrams($s1, $s2, $N);
 my $ngp = ngramPrecision($s1, $s2, $N);

 my @weights = (.5, .3, .2);
 my $wngp = weightedNgramPrecision($s1, $s2, $N, @weights);

=head1 DESCRIPTION

List-based distance metrics.  All metrics take as input a pair of
references to lists.  Lists are assumed to contain scalar elements
only.

=cut

package CE::distance;

use strict;
use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = '0.1';
@ISA         = qw(Exporter);
@EXPORT      = ();
@EXPORT_OK   = qw(&Levenshtein &longestCommonSubstring &ngramPrecision &weightedNgramPrecision &commonNgrams);
%EXPORT_TAGS = ();


sub _min
{
	return $_[0] < $_[1]
		? $_[0] < $_[2] ? $_[0] : $_[2]
		: $_[1] < $_[2] ? $_[1] : $_[2];
}

# Return 1 if s and t are equal; 0 otherwise
sub arraysEqual {
    my ($s, $t) = @_;              # s and t are array refs
    return 0 if ($#$s != $#$t);

    for my $i (0 .. $#$s) {
        return 0 if $s->[$i] ne $t->[$i];
    }
    
    return 1;
}

=pod 

=head1 FUNCTIONS

=cut

=pod 

=head2 Levenshtein

Returns the Levenshtein distance between two lists, assuming a cost of
1 for insertions, deletions and substitutions.

=cut

# Return the Levenshtein distance between s and t
sub Levenshtein
{
    my ($s,$t)=@_;              # s and t are array refs

    return 0 if arraysEqual($s,$t);

    my $n=scalar @$s;
    my $m=scalar @$t;
    return $m unless $n;
    return $n unless $m;

    my @d;
    my $cost=0;

    $d[0][0]=0;
    foreach my $i (1 .. $n) {
        $d[$i][0]=$i;
    }
    foreach my $j (1 .. $m) {
        $d[0][$j]=$j;
    }

    for my $i (1 .. $n) {
        for my $j (1 .. $m) {
            $d[$i][$j]=&_min($d[$i-1][$j]+1,
                             $d[$i][$j-1]+1,
                             $d[$i-1][$j-1]+($s->[$i-1] eq $t->[$j-1] ? 0 : 1) )
            }
    }

    return $d[$n][$m];
}

=pod 

=head2 longestCommonSubstring

Returns the length of the longest common substring (sequence of
contiguous elements) between two lists.

=cut


# Return the length of the longest common substring between s and t
sub longestCommonSubstring {
    my ($s,$t)=@_;              # s and t are array references

    my $n=scalar @$s;
    my $m=scalar @$t;
    return 0 if ($n == 0 or $m == 0);

    my $len = [];
    my $maxlen = 0;

    for my $i (0..$n-1) {
        push @$len, [];          # add a row
        for my $j (0..$m-1) {
            push @{$len->[$i]}, 0;# add a column
            if ($s->[$i] eq $t->[$j]) {
                if ($i == 0 or $j == 0) {
                    $len->[$i][$j] = 1; # begin a substring
                } else {
                    $len->[$i][$j] = $len->[$i-1][$j-1] + 1; # continue a substring
                }
                $maxlen = $len->[$i][$j] if $len->[$i][$j] > $maxlen;
            }
        }
    }
    return $maxlen;
}


use List::Util qw(min max);

=pod 

=head2 commonNgrams

Returns the number of common Ngrams (contiguous sequence of N
elements) between two lists, for given N.

=cut

# Return the number of common N-grams between test and ref
sub commonNgrams {
    my ($test, $ref, $N) = @_;              # test and ref are array references

    my %S_test = ();
    for my $i (0..$#$test - $N + 1) {
        my $ngram = join(" ", @$test[$i..$i+$N-1]);
        $S_test{$ngram} = 0 unless exists $S_test{$ngram};
        $S_test{$ngram} += 1;
    }

    my %S_ref = ();
    for my $i (0..$#$ref - $N + 1) {
        my $ngram = join(" ", @$ref[$i..$i+$N-1]);
        $S_ref{$ngram} = 0 unless exists $S_ref{$ngram};
        $S_ref{$ngram} += 1;
    }

    my $sum = 0;
    while (my ($ngram, $count) = each %S_ref) {
        $sum += min($count, $S_test{$ngram}) if exists $S_test{$ngram};
        # print "+ $ngram: ", min($count, $S_test{$ngram}), "\n" if exists $S_test{$ngram};
    }

    return $sum;
}

=pod 

=head2 ngramPrecision

Returns the number of common Ngrams (contiguous sequence of N
elements) between two lists, divided by the total number of Ngrams in
the first list, for given N.

=cut

# Return the N-gram precision of ref relative to test ( common N-grams / N-grams in test )
sub ngramPrecision {
    my ($test, $ref, $N) = @_;              # test and ref are array references

    my $sum = commonNgrams($test, $ref, $N);
    # print "= $sum\n";

    my $test_ngrams = max(0, scalar(@$test) - $N + 1);
    # print "/ $test_ngrams =\n";

    return $test_ngrams == 0 ? 0 : $sum / $test_ngrams;
}

# Computes n-gram precision of ref relative to test, for all n <= N, and returns a weighted average. 
=pod 

=head2 weightedNgramPrecision

Returns a weighted average of n-gram precisions (see L</ngramPrecision>), for values of n in (1..N). N defaults to 4; weights default to 1/N. 

=cut

sub weightedNgramPrecision {
    my ($test, $ref, $N, @weights) = @_;              # test and ref are array references
    $N = 4 unless $N;
    @weights = () unless @weights;
    while ($#weights + 1 < $N) {
        push @weights, 1/$N;
    }

    my $p = 0;
    for my $n (1..$N) {
        $p += $weights[$n-1] * ngramPrecision($test, $ref, $n);
    }

    return $p;
}




1;

=pod

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

Technologies langagieres interactives / Interactive Language Technologies
Inst. de technologie de l'information / Institute for Information Technology
Conseil national de recherches Canada / National Research Council Canada
Copyright 2009, Sa Majeste la Reine du Chef du Canada /
Copyright 2009, Her Majesty in Right of Canada

=cut
