# @file feature.pm
# @brief Feature functions for confidence estimation.
#
# @author Michel Simard
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

=pod

=head1 NAME

CE::feature

=head1 SYNOPSIS

 use CE::feature;

See individual methods for details.

=head1 DESCRIPTION

This packages manages most aspects related to features for confidence
estimation (I<CE>) in PORTAGE.

=cut

package CE::feature;

use strict;
use warnings;
use open IO=>':utf8';
use filetest 'access';
use File::Spec qw(rel2abs);
use CE::distance qw(Levenshtein longestCommonSubstring commonNgrams);
use CE::help;
use Carp;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter CE::base);
@EXPORT      = ();
@EXPORT_OK   = qw(new name type isTarget setArgs getArgs addArgs arg generate depend);
%EXPORT_TAGS = ();

our $SRC_LANG="en";
our $TGT_LANG="fr";
our $SILENCER=""; ## " 2> /dev/null";

my $GEN="gen_feature_values";
my $BLEU="bleumain";
my $WER="wermain";

=pod

=head1 FUNCTIONS

=head2 new($name [, %args])

Create a new feature object. Generic arguments are:

=over

=item C<target=E<gt>1> :    If this is a "target" feature (used as label for classification/regression)

=item C<type=E<gt>$type_name> :  When feature type is different from its name (currently unused)

=back

Other feature-specific arguments may be set.  See section L</"CE Features"> for details.

=cut

sub new {
    my ($this, $name, %args) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->initialize($name, %args);
    return $self;
}

sub initialize {
    my ($this, $name, %args) = @_;

    undef %$this;

    die "Error: Unspecified feature name in initialize()" unless defined $name;
    $this->name($name);
    $this->type(exists($args{type}) ? $args{type} : $name);
    $this->isTarget(exists($args{target}) ? $args{target} : 0);
    $this->setArgs(%args);

    return $this;
}

=pod

=head2 name( [$name] )

Set/get this feature's name.

=cut

sub name {
    my ($this, $name) = @_;

    $this->{name} = $name if defined $name;

    return $this->{name};
}

=pod

=head2 type( [$type] )

Set/get this feature's type

=cut

sub type {
    my ($this, $type) = @_;

    $this->{type} = $type if defined $type;

    return $this->{type};
}


=pod

=head2 isTarget( [$yesno] )

Check/set whether this is a target feature (used as label in regression/classification).

=cut

sub isTarget {
    my ($this, $yesno) = @_;

    $this->{isTarget} = $yesno if defined $yesno;

    return $this->{isTarget};
}


=pod

=head2 setArgs( %args )

Destructively set feature-specific arguments; see section L</"CE Features"> for
details about arguments.  This method deletes all previous argument settings; see
function C<addArgs> for a non-destructive alternative.

=cut

sub setArgs {
    my ($this, %args) = @_;

    $this->{args} = { %args };

    return %{$this->{args}};
}


=pod

=head2 getArgs( )

Get feature-specific arguments as a hash; see section L</"CE Features"> for details.

=cut

sub getArgs {
    my ($this) = @_;

    return %{$this->{args}};
}


=pod

=head2 addArgs

Non-destructively set feature-specific arguments; see section L</"CE
Features"> for details about arguments.  This method does not delete
previous argument settings, but it may override previous settings for
specific arguments; see function C<setArgs> for a destructive
alternative.

=cut

sub addArgs {
    my ($this, %args) = @_;

    $this->{args} = { $this->getArgs(), %args };

    return %{$this->{args}};
}


=pod

=head2 arg($key [, $val])

Get/set the value of a feature-specific argument.

=cut

sub arg {
    my ($this, $key, $val) = @_;

    die "Error: Unspecified $key in feature arg()" unless defined $key;

    if (defined $val) {
        $this->{args}{$key} = $val;
        return $val;
    } else {
        return exists($this->{args}{$key}) ? $this->{args}{$key} : undef;
    }
}

=pod

=head2 generate

Generate feature values:

    $feature->generate(%args ...);

See section L</"CE Features"> for a detailed description
of the features that this function can compute.  A
number of named arguments can be specified that affect the
behavior of this function:

=over

=item C<dir=E<gt>'work_dir'>

The values of the feature are written to a file with the name of that
feature. By default, this is done in the current directory.  If the
C<dir> argument is specified, then feature C<feat> is written to file
F<work_dir/feat>.

=item C<path=E<gt>'colon_separated_search_path'>

Some features require filename arguments, e.g. C<p.lm> which requires
a C<lm=file> argument. If file is given as a relative path,
e.g. "plain_filename" or "../relative/filename", then the file is
normally searched for relative to the current working directory (see
C<dir> above). If C<path> is specified, however, then this
(colon-separated) list of directories C<search_path> is searched, in
the given order.

=item C<force=E<gt>1>

If the feature's file already exists, this function assumes that it
has previously been computed and does not recompute its value, unless
the C<force> argument as a non-null value.

=item C<depend=E<gt>1>

If the feature is checked for dependency.

=back

=cut

# Generate individual feature values for the text in the current work directory.
sub generate {
    my ($feature, %args) = @_;

    my $dir = $args{dir} || ".";
    my $search_path = defined($args{path}) ? $args{path} : $dir;
    my @search_path = split(/:/, $search_path);
    my $prefix = $args{prefix} || "";
    my $force = $args{force} || 0;
    my $depend = $args{depend} || 0;
    my $type = $feature->type();

    my $fout = "${dir}/${prefix}".$feature->name();

    # Feature file already exists: don't recompute unless in "force" mode
    if (not $force and -r $fout) {

        # Features that are known not to be generated by this program
    } elsif ($type eq 'qs.sim'
        or $type eq 'p.oov'
        or $type eq 'p.score'
        or $type eq 'p.plen'
        or $type eq 'p.pmax') {
        if (not -r $fout) {
            if ($depend) {
                die "Error: Missing feature value file for feature $type: $fout";
            } else {
                warn "Warning: Missing feature value file for feature $type: $fout";
            }
        }
        # Any feature whose values must be read from a file
    } elsif ($feature->arg('file')) {
        my $fin = findFile($feature->arg('file'), @search_path);
        my $field = $feature->arg('field') || 1;
        call("cut -f ${field} \"${fin}\"", $fout);

        # Length in characters
    } elsif ($type =~ /^([qptr])\.clen$/) {
        my $x = $1;
        my $fin = "${dir}/${prefix}${x}.tok";
        call("$GEN LengthFF 0 \"${fin}\" \"${fin}\"", $fout);

        # Length in words
    } elsif ($type =~ /^([qptr])\.wlen$/) {
        my $x = $1;
        my $fin = "${dir}/${prefix}${x}.tok";
        open(my $in, "< ${fin}") or die "Error: Can't open ${fin}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $line=<$in>) {
            chop $line;
            my $wlen = my @tokens = split(/\s+/, $line);
            print $out $wlen, "\n";
        }
        close $in;
        close $out;

        # Language model log prob
    } elsif ($type =~ /^([qptr])\.lm$/) {
        my $x = $1;
        die "Error: Missing feature argument: lm" unless $feature->arg('lm');
        my $lm = findFile($feature->arg('lm'), @search_path);
        my $fin = "${dir}/${prefix}${x}.tok";
        call("$GEN NgramFF \"${lm}\" \"${fin}\" \"${fin}\"", $fout);

        # Length normalized one-text feature
    } elsif ($type =~ /^([qptr])\.(.*)\.norm$/) {
        my ($x,$f) = ($1,$2);
        my $fin1 = $feature->depend("${x}.${f}", %args);
        my $fin2 = $feature->depend("${x}.wlen", %args);
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $value=<$in1>) {
            my $M=<$in2> || cleanupAndDie("Too few input lines in ${fin2}", $fout);
            my $norm_value = $value / ($M + 0.0001);
            print $out $norm_value, "\n";
        }
        cleanupAndDie("Too many input lines in ${fin2}", $fout) unless eof($in2);
        close $in1;
        close $in2;
        close $out;

        # Length normalized two-text feature
    } elsif ($type =~ /^([qptr])([qptr])\.(.*)\.norm$/) {
        my ($x,$y,$f) = ($1,$2,$3);
        my $fin1 = $feature->depend("${x}${y}.${f}", %args);
        my $fin2 = $feature->depend("${x}.wlen", %args);
        my $fin3 = $feature->depend("${y}.wlen", %args);
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $in3, "< ${fin3}") or die "Error: Can't open ${fin3}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $value=<$in1>) {
            my $M=<$in2> || cleanupAndDie("Too few input lines in ${fin2}", $fout);
            my $N=<$in3> || cleanupAndDie("Too few input lines in ${fin3}", $fout);
            my $norm_value = $value / ($M + $N + 0.0001);
            print $out $norm_value, "\n";
        }
        cleanupAndDie("Too many input lines in ${fin2}", $fout) unless eof($in2);
        cleanupAndDie("Too many input lines in ${fin3}", $fout) unless eof($in3);
        close $in1;
        close $in2;
        close $in3;
        close $out;

        # Length ratios
    } elsif ($type =~ /^([qptr])([qptr])\.(c|w)len\.ratio$/) {
        my ($x, $y) = ($1, $2);
        my $unit = $3;

        my $fin1 = $feature->depend("${x}.${unit}len", %args);
        my $fin2 = $feature->depend("${y}.${unit}len", %args);
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $len1=<$in1>) {
            my $len2=<$in2> || cleanupAndDie("Too few input lines in ${fin2}", $fout);
            my $ratio = $len1/($len2+.01);
            print $out $ratio, "\n";
        }
        cleanupAndDie("Too many input lines in ${fin2}", $fout) unless eof($in2);
        close $in1;
        close $in2;
        close $out;

        # Translation model probs
    } elsif ($type =~ /^q([ptr])\.(ibm1|ibm2|hmm)(\.rev)?$/) {
        my ($x, $y) = ("q", $1);
        my $tm = $2;
        my $rev = $3 || "";
        my $fin1 = "${dir}/${prefix}${x}.tok";
        my $fin2 = "${dir}/${prefix}${y}.tok";
        my $ff = ($tm eq 'ibm1' ? 'IBM1' :
                  $tm eq 'ibm2' ? 'IBM2' :
                  'HMM');
        $ff .= ($rev ? 'SrcGivenTgt' : 'TgtGivenSrc');
        die "Error: Missing feature argument: tm" unless $feature->arg('tm');
        my $model = findFile($feature->arg('tm'), @search_path);
        call("$GEN ${ff} \"${model}\" \"${fin1}\" \"${fin2}\"", $fout);

        # Translation model ratios
    } elsif ($type =~ /^([ptr])([ptr])\.(ibm1|ibm2|hmm)(\.rev)?\.ratio$/) {
        my ($x, $y) = ($1, $2);
        my $tm = $3;
        my $rev = $4 || "";
        my $fin1 = $feature->depend("q${x}.${tm}${rev}", %args);
        my $fin2 = $feature->depend("q${y}.${tm}${rev}", %args);
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $p1=<$in1>) {
            my $p2=<$in2> || cleanupAndDie("Too few input lines in ${fin2}", $fout);
            my $ratio = $p1 - $p2; # these are log probs, really
            print $out $ratio, "\n";
        }
        cleanupAndDie("Too many input lines in ${fin2}", $fout) unless eof($in2);
        close $in1;
        close $in2;
        close $out;

        # Levenshtein distance
    } elsif ($type =~ /^([qptr])([qptr])\.lev$/) {
        my ($x, $y) = ($1, $2);
        my $fin1 = "${dir}/${prefix}${x}.tok";
        my $fin2 = "${dir}/${prefix}${y}.tok";
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $line1=<$in1>) {
            chop $line1;
            my @t1 = split(/\s+/, $line1);
            my $line2 = readline($in2);
            cleanupAndDie("Not enough lines in $fin2", $fout) unless defined $line2;
            chop $line2;
            my @t2 = split(/\s+/, $line2);
            my $lev = Levenshtein(\@t1, \@t2);
            print $out $lev, "\n";
        }
        close $in1;
        close $in2;
        close $out;

       # Longest common substring
    } elsif ($type =~ /^([qptr])([qptr])\.lcs$/) {
        my ($x, $y) = ($1, $2);
        my $fin1 = "${dir}/${prefix}${x}.tok";
        my $fin2 = "${dir}/${prefix}${y}.tok";
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $line1=<$in1>) {
            chop $line1;
            my @t1 = split(/\s+/, $line1);
            my $line2 = readline($in2);
            cleanupAndDie("Not enough lines in $fin2", $fout) unless defined $line2;
            chop $line2;
            my @t2 = split(/\s+/, $line2);
            my $lcs = longestCommonSubstring(\@t1, \@t2);
            print $out $lcs, "\n";
        }
        close $in1;
        close $in2;
        close $out;

        # NGram count
    } elsif ($type =~ /^([qptr])([qptr])\.(\d+)gram$/) {
        my ($x, $y) = ($1, $2);
        my $n = $3;
        my $fin1 = "${dir}/${prefix}${x}.tok";
        my $fin2 = "${dir}/${prefix}${y}.tok";
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $line1=<$in1>) {
            chop $line1;
            my @t1 = split(/\s+/, $line1);
            my $line2 = readline($in2);
            cleanupAndDie("Not enough lines in $fin2", $fout) unless defined $line2;
            chop $line2;
            my @t2 = split(/\s+/, $line2);
            my $ng_cnt = commonNgrams(\@t1, \@t2, $n);
            print $out $ng_cnt, "\n";
        }
        close $in1;
        close $in2;
        close $out;

        # Usefulness
    } elsif ($type =~ /^([qptr])([qptr])\.u(\d+)$/) {
        my ($x, $y, $K) = ($1, $2, $3);
        my $fin1 = $feature->depend("${x}${y}.lcs", %args);
        my $fin2 = $feature->depend("${x}.wlen", %args);
        open(my $in1, "< ${fin1}") or die "Error: Can't open ${fin1}";
        open(my $in2, "< ${fin2}") or die "Error: Can't open ${fin2}";
        open(my $out, "> ${fout}") or die "Error: Can't open ${fout}";
        while (my $lcs=<$in1>) {
            my $N = <$in2> || cleanupAndDie("Too few input lines in ${fin2}", $fout);
            my $U = (min($lcs, $K)/$K) * ($N > 0 ? (min(2*$lcs,$N)/$N) : 0);
            print $out $U, "\n";
        }
        cleanupAndDie("Too many input lines in ${fin2}", $fout) unless eof($in2);
        close $in1;
        close $in2;
        close $out;

        # smoothed BLEU
    } elsif ($type =~ /^([qptr])([qptr])\.bleu$/) {
        my ($x, $y) = ($1, $2);
        my $fin1 = "${dir}/${prefix}${x}.tok";
        my $fin2 = "${dir}/${prefix}${y}.tok";
        call("${BLEU} -detail 1 \"${fin1}\" \"${fin2}\" | grep 'Sentence.*BLEU score:' | cut -f5 -d' '", $fout);
        # WER
    } elsif ($type =~ /^([qptr])([qptr])\.wer$/) {
        my ($x, $y) = ($1, $2);
        my $fin1 = "${dir}/${prefix}${x}.tok";
        my $fin2 = "${dir}/${prefix}${y}.tok";
        call("${WER} -detail 1 \"${fin1}\" \"${fin2}\" | grep 'Sentence.*WER score:' | cut -f5 -d' '", $fout);
        # Unknown feature: die
    } else {
        die "Error: Can't handle feature $type";
    }

    return $fout;
}

# Check for dependent feature -- return the name of the file from which to read features;
sub depend {
    my ($feature, $depends_on, %args) = @_;
    my $df = $feature->new($depends_on, $feature->getArgs());
    return $df->generate(%args, force=>0, depend=>1);
}

## Utilities

sub min { my $min=shift @_; for my $x (@_) { $min = $x if $x<$min }; return $min }
sub max { my $max=shift @_; for my $x (@_) { $max = $x if $x>$max }; return $max }
sub trim{ for my $s (@_) { $s =~ s/^\s+|\s+$//g; } return @_ }

sub findFile {
    my ($filename, @search_path) = @_;

    for my $path (@search_path) {
        my $filepath = File::Spec->rel2abs($filename, $path);
        return $filepath if (-r $filepath);
    }

    confess sprintf("Can't find $filename in directories: %s", join(", ", @search_path));

    return undef;
}

# https://stackoverflow.com/questions/10729015/pass-array-and-scalar-to-a-perl-subroutine
sub call {
    my ($cmd, $fout) = @_;
    system("${cmd} 1> ${fout} $SILENCER") == 0
        or cleanupAndDie("Command failed: $cmd", $fout);
}

sub cleanupAndDie {
    my ($message, @files) = @_;

    unlink @files;
    die "Error: ", $message;
}


CE::help::keyword("features", __PACKAGE__."::CE Features");

=pod

=head1 CE Features

In the following description, C<X>, C<Y> and C<Z> stand for text
identifiers, and can be replaced by C<q>, C<p>, C<r>, C<s> or C<t>,
where:

=over

=item - C<q> refers to the source-language text

=item - C<p> refers to the target-language machine translation

=item - C<s> refers to the best source-language translation memory match

=item - C<t> refers to the best target-language translation memory match

=item - C<r> refers to the target language reference translation)

=back

The following features are computed by this program.

=over

=item C<X.clen>

Length in characters of tokenized text

=item C<X.wlen>
.
Length in words (token count)

=item C<XY.{clen,wlen}.ratio>

Character/Word-length ratios

=item C<X.lm:lm=F>

Language model log probability I<P(X)>, using LM in file F<F>

=item C<XY.{ibm1,ibm2,hmm}{.rev}:tm=F>

Translation model I<P(Y|X)> probability (P(X|Y) if C<.rev>), using TM
file F<F>

=item C<YZ.{ibm1,ibm2,hmm}{.rev}.ratio:tm=F>

Translation model ratio S<I<P(Y|q) / P(Z|q)>>; see the notes below on
feature dependencies

=item C<XY.lev>

Word-based Levenshtein distance between C<X> and C<Y>

=item C<XY.lcs>

Longest common substring between C<X> and C<Y>

=item C<XY.I<N>gram>

I<N>-gram matches between C<X> and C<Y>, for numeric value of I<N>

=item C<XY.bleu>

per-sentence smoothed BLEU score

=item C<XY.wer>

per-sentence word error rates

=item C<XY.uI<N>>

Usefulness, based on I<N>-word LCS.

=item C<X.*.norm>

=item C<XY.*.norm>

length-normalized feature value: one-text features
(e.g. C<X.lm.norm>) will be divided by the word count of C<X> (C<X.wlen>),
two-text features (e.g. C<XY.lev.norm>) by the sum of the word
counts. Features that normally requires an argument (eg C<X.lm>)
don't need to provide it if the non-normalized feature appears
B<prior> to its normalized version.

=back

Note that any feature whose name does not match the above list will
simply be ignored by this program, with the exception of features with
a C<file=F> argument, as discussed above.  (So beware of typos!)

=cut

CE::help::keyword("depend", __PACKAGE__."::CE Feature Dependencies");

=pod

=head1 CE Feature Dependencies

Most features require that tokenized-lowercased version of the texts
(files F<X.tok> exist in the current work directory.

Some features are computed based on the values of others.  Most
notable cases are the C<.ratio> and C<.norm> features.  Insofar as this is
possible, dependencies between features are handled automatically, so
that if feature I<A> depends on feature I<B>, I<B> will first be
computed and saved to file, then I<A> will be computed based on the
value of I<B>.  This is done regardless of whether or not I<B> is part
of the model. If I<B> is part of the model, and appears prior to I<A>
in the model description file, then its value will not be recomputed
when computing I<A>.

Of course, this only applies in the case where this program knows how
to compute I<B>.  If this is not the case, then you need to make sure
that a file called F<B> exists, containing values for that feature,
prior to calling this program.

It is also important to note that currently, multiple features of the
same type are not supported.  Of course, this would only make sense
for features that require arguments, such as C<X.lm>; unfortunately, this
is not currently possible.  An important side-effect of this
limitation occurs with dependent features that require arguments (eg
C<X.lm> and C<X.lm.norm>): if both I<A> and I<B> appear in the model, and if
I<A> depends on I<B>, then the argument(s) of whichever is specified
first will be used for computing the values of both.  For example, if
you have:

=over

=item C<q.lm:lm=lmfile1>

=item C<q.lm.norm:lm=lmfile2>

=back

then the argument C<lm=lmfile2> will be ignored when computing
C<q.lm.norm>, because the computation will be done using the values of
C<q.lm> obtained with language model F<lmfile1>.  The opposite will
happen if you swap the two features in the file. You could call this a
bug.

=head1 SEE ALSO

L<CE_Normalize>, L<CE_libsvm>, L<CE_Distance>.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

Technologies langagieres interactives / Interactive Language Technologies
Inst. de technologie de l'information / Institute for Information Technology
Conseil national de recherches Canada / National Research Council Canada
Copyright 2009, Sa Majeste la Reine du Chef du Canada /
Copyright 2009, Her Majesty in Right of Canada

=cut

1;
