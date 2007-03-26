#!/usr/bin/perl -w
# $Id$

# cvs-status-summary.pl - Make the output of cvs status more compact, making
#                         the important information stand out more
#
# PROGRAMMER: Eric Joanis
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

print STDERR "cvs-status-summary.pl, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada\n";

sub usage {
    local $, = "\n";
    print STDERR @_, "";
    $0 =~ s#.*/##;
    print STDERR "
Usage: cvs status | cvs-status-summary.pl [options]
Options: -n(ew)  skip up-to-date files.
         -h(elp) print this help message
";
    exit 1;
}

use Getopt::Long;
GetOptions(
    "new"       => \my $newonly,
    help        => sub { usage },
);

use strict;
local $/ = "=================\n";
while (<>) { 
    next unless /revision/;
    my ($status) = /Status:\s+(.*)/;
    next if $newonly && $status =~ /Up-to-date/;
    $status =~ s/Up-to-date//;
    my ($working) = /Working revision:\s+(\S+)/;
    my ($date) = /Working revision:\s+\S+\s+(.*)/;
    my ($repository, $filename) = 
        /Repository revision:\s+(\S+)\s+\S+Portage\/(.*)/;
    if ( ! defined $repository ) {
        # Probably a new file
        $repository = "Unknown";
        ($filename) = /File:\s+(\S+)/;
    }
    if ($repository eq $working) { $repository = "" }
    my ($sticky_tag) = /Sticky Tag:\s*(.*)/;
    my ($sticky_date) = /Sticky Date:\s*(.*)/;
    my ($sticky_opts) = /Sticky Options:\s*(.*)/;
    for ($sticky_tag, $sticky_date, $sticky_opts) {
        if ( defined $_ ) {
            s/\(none\)//;
        } else {
            $_ = "";
        }
    }
    my $sticky = "";
    if ($sticky_tag || $sticky_date || $sticky_opts) {
        $sticky = "Sticky: $sticky_tag $sticky_date $sticky_opts";
    }
    printf "%-8s %-24s   %-8s %-16s %s   %s\n",
        $working, $date, $repository, $status, $sticky, $filename;
}
