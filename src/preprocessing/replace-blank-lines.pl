#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;

# @file replace-blank-lines.pl 
# @brief Write to stdout a copy of main-trans in which blank lines have been
# replaced by corresponding lines in backup-trans.
#
# @author George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

use strict;
use warnings;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils";
   }
}
use portage_utils;
printCopyright("replace-blank-lines.pl", 2006);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $HELP =
"Usage: replace-blank-lines.pl main-trans backup-trans

Write to stdout a copy of main-trans in which blank lines have been replaced by
corresponding lines in backup-trans.

";

our ($h, $help);

if (defined $h || defined $help)
{
    print $HELP;
    exit;
} # if

my $mainfile = shift || die "Error: Missing main-trans\n$HELP";
my $backfile = shift || die "Error: Missing back-trans\n$HELP";

open(MAIN, "<$mainfile") or die "Error: Can't open $mainfile for reading\n";
open(BACK, "<$backfile") or die "Error: Can't open $backfile for reading\n";

my $mainline;
while ($mainline = <MAIN>) {

    my $backline;

    if (!($backline = <BACK>)) {die "Error: $backfile too short!\n";}
    
    if ($mainline !~ /^\s*$/o) {print $mainline;}
    else {print $backline;}
}

if (<BACK>) {die "Error: $mainfile too short!\n";}
