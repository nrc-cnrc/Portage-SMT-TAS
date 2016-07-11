# @file data.pm 
# @brief Datapoint representation for confidence estimation
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

CE::data 

=head1 SYNOPSIS

 use CE::data;

 # constructor
 my $d1 = CE::data->new(); # x = (), y= undef
 my $d2 = CE::data->new(0,1,2,3,4,5); # x=(1,2,3,4,5), label y=0

 # copy:
 my $d3 = $d2->copy();

 # set/get label
 $d2->label(2); 
 my $d2_label = $d2->label();

 # set get x values:
 $d2->value(1,6); # set x1 to value 6
 my $d2_x1 = $d2->value(1); # get value of x1
 my $d2_x0 = $d2->value(0); # ERROR: value indices begin at 1
 my $d2_size = $d2->size(); # should be 5
 my $d2_x1 = $d2->value(7, -1); # sets value of x7 to -1.  Beware: x6 is undef!

 # get values as array:
 my @x2 = $d2->asArray();
 

=head1 DESCRIPTION

A datapoint, from a classification/regression perspective: a vector of values, and a label.

=cut

package CE::data;

use strict;
use warnings;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = qw();
@EXPORT_OK   = @EXPORT;
%EXPORT_TAGS = ( functions => [qw(new copy label value size asArray)],
                 vars => [ ]);

=pod

=head1 FUNCTIONS

=head2 new($label, @values)

=cut 

sub new {
    my ($this, $label, @values) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->initialize($label, @values);
    return $self;
}

=pod

=head2 initialize($label, @values)

=cut 

sub initialize {
    my ($this, $label, @values) = @_;

    undef %$this;

    $this->label($label);

    $this->{values} = [ @values ];

    return $this;
}

=pod

=head2 copy( )

=cut 

sub copy {
    my ($this) = @_;

    return $this->new($this->label(), $this->asArray());
}

=pod

=head2 label([ $label ])

=cut 

# get/set label of data 
sub label {
  my ($this, $label) = @_;

  return defined($label) ? $this->{label} = $label + 0 : $this->{label};
}

=pod

=head2 value($index [ , $value ])

Note: index >= 1

=cut 

# get/set value
sub value {
  my ($this, $index, $value) = @_;

  die "Error: No index specified" unless defined $index;
  die "Error: Non-positive index" if $index < 1; 

  return (defined $value 
          ? $this->{values}[$index-1] = $value + 0 
          : $this->{values}[$index-1]);
}

=pod

=head2 size( )

=cut 

# get length of value vector
sub size {
  my ($this) = @_;

  return $#{$this->{values}}+1;
}

=pod

=head2 asArray( )

=cut 

# get vector of values, as an array
sub asArray {
  my ($this) = @_;

  return @{$this->{values}};
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
