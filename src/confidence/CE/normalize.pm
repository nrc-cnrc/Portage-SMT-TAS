# @file normalize.pm
# @brief Feature Normalization for confidence estimation
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

CE::normalize 

=head1 SYNOPSIS

 use CE::normalize qw(new initialize learn apply method toString);

 $minmax = CE::normalize::new('minmax', lower=>0, upper=>1);
 $minmax->learn(5, 4, 3, 2, 1);
 print $minmax->toString(); # prints out "minmax(0,1,1,5)"
 @norm_test_values = $minmax->apply(@test_values);

 $zm1 = CE::normalize::new('zeromean', mean=>-5, sd=>0.42); 
 $zm2 = CE::normalize::new($zm1->toString()); # Copy

=head1 DESCRIPTION

Feature normalization for confidence estimation.  Implements zero-mean
and min-max normalization.

=cut

package CE::normalize;

use strict;
use warnings;
# use Algorithm::SVM::DataSet;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter CE::base);
@EXPORT      = ();
@EXPORT_OK   = qw(new method learn apply toString fromString);
%EXPORT_TAGS = ();

our $verbose = 0;
our $debug = 0;

=pod

=head1 FUNCTIONS

=head2 new( $desc[, %args ] )

Construct a normalizer from the given descriptor and
optional arguments. Descriptor $desc has the following syntax:

  DESC ::= none | zeromean[ZM_ARGS] | minmax[MM_ARGS]
  ZM_ARGS ::= ( ZM_MEAN [, ZM_SD] )
  MM_ARGS ::= ( MM_LOWER [, MM_UPPER [, MM_MIN [, MM_MAX ] ] ] )

Where ZM_MEAN, ZM_SD, MM_LOWER, MM_UPPER, MM_MIN, MM_MAX are numerical
values.  If any of these is unspecified, default values are as
follows:

 ZM_MEAN = 0
 ZM_SD = 1
 MM_LOWER = MM_MIN = -1
 MM_UPPER = MM_MAX = 1

Descriptor parameters (ZM_MEAN, ZM_SD, etc.) can alternatively be
provided as arguments in %args.  Recognized argument names are
C<mean>, C<sd>, C<lower>, C<upper>, C<min> and C<max>.  For example,
the following are equivalent:

 my $n1 = CE::normalize->new("zeromean(-1.5, 3)");
 my $n2 = CE::normalize->new("zeromean", sd=>3, mean=>-1.5);
 my $n3 = CE::normalize->new("zeromean(-1.5)", sd=>3);

=cut 

sub new {
    my ($this, $desc, %args) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->initialize($desc, %args);
    return $self;
}

sub initialize {
    my ($this, $desc, %args) = @_;

    undef %$this;

    $this->fromString($desc);
    for my $arg (keys %args) {
        if ((($this->method() eq 'minmax') 
             and $arg =~ /^(lower|upper|min|max)$/)
            or
            (($this->method() eq 'zeromean') 
             and $arg =~ /^(mean|sd)$/)) {
            $this->{$arg} = 0+$args{$arg};
        } else {
            my $method = $this->method();
            die "Error: Unknown arg $arg for $method normalization";
        }
    }

    return $this;
}

=pod

=head2 learn( @values )

Learn normalization parameters from the given values.

=cut 

sub learn {
    my ($this, @values) = @_;

    if ($this->method() eq 'minmax') {
        $this->learnMinMax(@values);
    } elsif ($this->method() eq 'zeromean') {
        $this->learnZeroMean(@values);
    } elsif ($this->method() eq 'none') {
    } else {
        die "Error: Unsupported normalization method ".$this->method();;
    }
    return $this;
}

=pod

=head2 apply( @values )

Return normalized values.

=cut 

sub apply {
    my ($this, @values) = @_;

    if ($this->method() eq 'minmax') {
        @values = $this->applyMinMax(@values);
    } elsif ($this->method() eq 'zeromean') {
        @values = $this->applyZeroMean(@values);
    } elsif ($this->method() eq 'none') {
    } else {
        die "Error: Invalid method";
    }
    return @values;
}

=pod

=head2 method( )

Get the name of the normalization method, one of
C<none>, C<minmax> or C<zeromean>.

=cut 

sub method {
    my ($this, $method) = @_;

    return $this->{method};
}



=pod

=head2 toString( )

Returns a string describing the normalization method and parameters,
which can be decoded by method C<fromString> and given as argument to
C<new> constructor.  This can be used e.g. to copy a CE::normalize
object:

 my $n2 = CE::normalize->new($n1->toString());

=cut 

sub toString {
    my ($this) = @_;

    return ($this->method() eq 'minmax' ? sprintf('minmax(%.6g,%.6g,%.6g,%.6g)', 
                                                $this->{lower}, $this->{upper}, 
                                                $this->{min}, $this->{max}) :
            $this->method() eq 'zeromean' ? sprintf('zeromean(%.6g,%.6g)', 
                                                  $this->{mean}, $this->{sd}) :
            'none');
}


=pod

=head2 fromString( $desc )

Initialize current CE::normalize object using string descriptor $desc.
This is the function called internally by constructor C<new>.  See
that method for details of descriptor syntax.

=cut 

sub fromString {
    my ($this, $desc) = @_;

    if ($desc =~ /^([\w\d]+)(?:\((.*)\))?$/) {
        my ($method, $args) = ($1, $2);
        my @v = $args ? split(/,/, $args) : ();
        $this->{method} = $method;
        if ($method eq 'minmax') {
            $this->{lower} = @v ? 0+shift @v : -1;
            $this->{upper} = @v ? 0+shift @v :  1;
            $this->{min} = @v ? 0+shift @v : -1;
            $this->{max} = @v ? 0+shift @v : 1;
            die "Error: Too many arguments for min-max normalization" if @v;
        } elsif ($method eq 'zeromean') {
            $this->{mean} = @v ? 0+shift @v : 0;
            $this->{sd} = @v ? 0+shift @v : 1;
            die "Error: Too many arguments for zero-mean normalization" if @v;
        }
    }
    return $this;
}

# Private

sub learnMinMax {
    my ($this, @values) = @_;
    my $min = undef;
    my $max = undef;
    for my $v (@values) {
        $min = $v if ((not defined $min) or ($min > $v));
        $max = $v if ((not defined $max) or ($max < $v));
    }
    $max = $min + 1 
        if ($max <= $min);
    $this->{min} = $min;
    $this->{max} = $max;
    return $this;
}

sub applyMinMax {
    my ($this, @values) = @_;

    for my $v (@values) {
        $v = (($v - $this->{min}) 
              / ($this->{max} - $this->{min})
              * ($this->{upper} - $this->{lower}) 
              + $this->{lower});
    }
    return @values;
}

sub learnZeroMean {
    my ($this, @values) = @_;

    my $sum=0;
    my $sum2=0;
    my $N = 0;

    for my $v (@values) {
        $N++;
        $sum += $v;
        $sum2 += $v*$v;
    }
    $this->{mean} = $sum / $N;
    $this->{sd} = sqrt(($sum2/$N) - ($this->{mean} * $this->{mean}));
    $this->{sd} = 1e-8 if ($this->{sd} < 1e-8);

    return $this;
}

sub applyZeroMean {
    my ($this, @values) = @_;

    for my $v (@values) {
        $v = ($v - $this->{mean}) / $this->{sd};
    }
    return @values;
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
