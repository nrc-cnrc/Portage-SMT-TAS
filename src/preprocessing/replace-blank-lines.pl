#!/usr/bin/perl -sw

# @file replace-blank-lines.pl 
# @brief Write to stdout a copy of main-trans in which blank lines have been
# replaced by corresponding lines in backup-trans.
#
# @author George Foster.
#
# Copyright (c) 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright (c) 2006, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "replace-blank-lines.pl, NRC-CNRC, (c) 2006 - 2009, Her Majesty in Right of Canada\n";

my $HELP =
"Usage: $0 main-trans backup-trans

Write to stdout a copy of main-trans in which blank lines have been replaced by
corresponding lines in backup-trans.

";

our ($h, $help);

if (defined $h || defined $help)
{
    print $HELP;
    exit;
} # if

my $mainfile = shift || die "missing main-trans\n$HELP";
my $backfile = shift || die "missing back-trans\n$HELP";

open(MAIN, "<$mainfile") or die "can't open $mainfile for reading\n";
open(BACK, "<$backfile") or die "can't open $backfile for reading\n";

my $mainline;
while ($mainline = <MAIN>) {

    my $backline;

    if (!($backline = <BACK>)) {die "$backfile too short!\n";}
    
    if ($mainline !~ /^\s*$/o) {print $mainline;}
    else {print $backline;}
}

if (<BACK>) {die "$mainfile too short!\n";}
