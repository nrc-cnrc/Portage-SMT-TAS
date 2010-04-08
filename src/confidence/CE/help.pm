#!/usr/bin/perl -s
# $Id$
# @file help.pm 
# @brief Help object for confidence estimation
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

CE::help 

=head1 SYNOPSIS

 use CE::help;

Associate POD section names with keywords:

 CE::help::keyword('thingy', 'Where is my thingy?');
 CE::help::keyword('gizmo', 'My::Package::What is a gizmo?');

Then get section text from the keyword:

 my $text = CE::help::text('gizmo');

=head1 DESCRIPTION

Keyword-based help through POD.

=cut

package CE::help;

use strict;
use warnings;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = ();
@EXPORT_OK   = qw(keyword text keywords);
%EXPORT_TAGS = ();

my %HELP = ();

=pod

=head1 FUNCTIONS

=head2 keyword

=cut 

sub keyword {
    my ($kw, $section) = @_;

    $HELP{$kw} = [ ] unless exists $HELP{$kw};

    my $help = { kw=>$kw, pkg=>"", title=>"" }; 

    my @parts = split(/::/, $section);
    $help->{title} = pop @parts || die "No section specified for keyword $kw";
    $help->{pkg} = @parts ? join("::", @parts) : $0;

    push @{$HELP{$kw}}, $help;
}

=pod

=head1 FUNCTIONS

=head2 keywords

=cut 

sub keywords {
     return keys %HELP;
}




=pod

=head2 text 

=cut

sub text { 
    my ($kw) = @_;
    
    my $matches = exists($HELP{$kw}) ? $HELP{$kw} : undef;

    return "" unless $matches;

    my $msg = "";
    for my $help (@$matches) {
        my $cmd = "perldoc -u ".$help->{pkg}." | podselect -section \"".$help->{title}."\" | pod2text";
        $msg .= `$cmd`;
    }
    return $msg;
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
