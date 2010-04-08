#!/usr/bin/perl -s
# $Id$
# @file dataset.pm 
# @brief Dataset representation for confidence estimation
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

CE::dataset 

=head1 SYNOPSIS

 use CE::dataset;

 # Constructor
 my $D = CE::dataset->new();

 # Add datapoints:
 $D->add(CE::data->new(0,1,2,3)); # append datapoint x=(1,2,3), y=0
 $D->add(...)

 # get/set datapoints or individual labels and values

 my $dfirst = $D->data(0);             # first CE::data object
 my $dlast = $D->data($D->size() - 1); # last in set
 $D->data(1, CE::data->new(5,4,3,2,1)); # replace an existing datapoint

 my $dfirst_y = $D->label(0);
 $D->label(0, 1);
 my $dfirst_x2 = $D->value(0, 2);
 $D->value(0, 2, 18);       
 
 # Get values as arrays:

 my @labels = $D->getLabels();
 my @dfirst = $D->getRow(0);
 my @x3 = $D->getColumn(3);

 # Copy the whole thing:

 my $D2 = $D->copy(); 


=head1 DESCRIPTION

Data set, from a classification/regression point of view, i.e. set of
labeled vectors.  Essentially just a list of CE::data objects, with
some added functionalities for convenience.

=cut

package CE::dataset;

use CE::data;

use strict;
use warnings;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = ();
@EXPORT_OK   = qw(new copy add size empty data value label getRow getColumn getLabels);
%EXPORT_TAGS = ( );

## DataSet

=pod

=head1 FUNCTIONS

=head2 new( )

Create a new dataset

=cut 

sub new {
    my ($this) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->initialize();
    return $self;
}

sub initialize {
    my ($this) = @_;

    undef %$this;

    $this->{data} = [ ];

    return $this;
}

=pod

=head2 copy( )

Return a copy of this dataset.

=cut 

sub copy {
    my ($this) = @_;

    my $copy = $this->new();
    for my $datapoint (@{$this->{data}}) {
        $copy->add($datapoint->copy());
    }

    return $copy;
}

=pod

=head2 add($data_object)

add a data object to this dataset

=cut 

sub add {
    my ($this, $data) = @_;

    die "Nothing to add" unless $data;
    die "Thing to add is not a CE::data" unless $data->isa('CE::data');

    push @{$this->{data}}, $data;
}

=pod

=head2 size( )

Return dataset current number of datapoints.

=cut 

sub size {
    my ($this) = @_;

    return $#{$this->{data}} + 1;
}


=pod

=head2 empty( )

Check if dataset is empty.

=cut 

sub empty {
    my ($this) = @_;

    return $this->size() <= 0;
}


=pod

=head2 data($k [, $data])

Get/set the k_th data object in this dataset; $k must be within range
(0, size-1); $data must be a CE::data object.

=cut 

sub data {
    my ($this, $k, $data) = @_;

    die "$k out of range [0, ".$this->size().")" unless 0 <= $k and $k < $this->size();
    if (defined $data) {
        die "Data argument is not a CE::data" unless $data->isa('CE::data');
        $this->{data}->[$k] = $data;
    }

    return $this->{data}->[$k];
}


=pod

=head2 value($i, $j [, $value]);

Get/set the j_th value of the i_th data object of this dataset.  $i
must be in range (0, size-1).  $j must be >= 1, but can be out of
range. $value must be numeric.

=cut 

sub value {
    my ($this, $k, $j, $value) = @_;

    my $data = $this->data($k);

    return $data->value($j, $value);
}


=pod

=head2 label($i [, $y])

Get/set the label of the i_th data object of this dataset  $i
must be in range (0, size-1). $value must be numeric.

=cut 

sub label {
    my ($this, $k, $label) = @_;

    my $data = $this->data($k);

    return $data->label($label);
}


=pod

=head2 getRow($i)

Get the values from the i_th data object in this dataset, as an array.  Equivalent to:

 $D->data($i)->asArray();

=cut 

sub getRow {
    my ($this, $k) = @_;

    my $data = $this->data($k);
    return defined $data ? $data->asArray() : undef;
}


=pod

=head2 getColumn($j)

Get the j_th value from each data object in dataset, as an array.

=cut 

sub getColumn {
    my ($this, $j) = @_;

    return map($_->value($j), @{$this->{data}});
}


=pod

=head2 getLabels( )

Get the labels from each data object in dataset, as an array.

=cut 

sub getLabels {
    my ($this) = @_;

    return map($_->label(), @{$this->{data}});
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
