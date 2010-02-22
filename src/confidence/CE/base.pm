#!/usr/bin/perl -s
# @file base.pm 
# @brief Base object for confidence estimation
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

CE::base 

=head1 SYNOPSIS

 use CE::base;

 my $obj = CE::base->new(verbose=>0, debug=>0);

 $obj->setVerbose(1);
 $obj->verbose("This message prints to STDERR only if verbose mode is set\n");
 $obj->debug("debug and verbose actually print through %s\n", "sprintf");
 my $debug_mode = $obj->getDebug();

=head1 DESCRIPTION

Basic CE object -- implements common methods (verbose and debug modes).

=cut

package CE::base;

use strict;
use warnings;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = ();
@EXPORT_OK   = qw(new verbose debug getVerbose getDebug setVerbose setDebug help);
%EXPORT_TAGS = ();

=pod

=head1 FUNCTIONS

=head2 new

Build a new base object. Optional arguments verbose and debug do the obvious thing:

  my $obj = CE::base->new(verbose=>1, debug=>0);

=cut 

sub new {
    my ($this, %args) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->initialize(%args);
    return $self;
}

=pod

=head2 initialize

=cut 

sub initialize {
    my ($this, %args) = @_;

    undef %$this;

    $this->setVerbose($args{verbose} || 0);
    $this->setDebug($args{debug} || 0);

    return $this;
}


=pod

=head2 verbose

When object is in verbose mode (see setVerbose), this method prints
out its arguments on STDERR, using sprintf. If not in verbose mode, it
does nothing.

=cut 

sub verbose {
    my ($this, @args) = @_;

    printf STDERR @args if $this->setVerbose();
}

=pod

=head2 setVerbose()



=cut 

sub setVerbose {
    my ($this, $value) = @_;

    $this->{'__VERBOSE__'} = $value if defined $value;
    return $this->{'__VERBOSE__'};
}

=pod

=head2 getVerbose()

=cut 

sub getVerbose {
    my ($this) = @_;

    return $this->{'__VERBOSE__'};
}

=pod

=head2 debug()

=cut 

sub debug {
    my ($this, @args) = @_;

    printf STDERR @args if $this->setDebug();
}

=pod

=head2 setDebug()

=cut 

sub setDebug {
    my ($this, $value) = @_;

    $this->{'__DEBUG__'} = $value if defined $value;
    return $this->{'__DEBUG__'};
}

=pod

=head2 getDebug()

=cut 

sub getDebug {
    my ($this) = @_;

    return $this->{'__DEBUG__'};
}

# Help on specific topics: extracted from the POD!
sub help { 
    my ($package, $title) = @_;
    my $cmd = "perldoc -u $package | podselect -section \"$title\" | pod2text";
    return `$cmd`;
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
