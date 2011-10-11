#!/usr/bin/env perl
# $Id$
# @file cow-timing.pl
# @brief Provide timing information for a cow run.
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

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
printCopyright "cow-timing.pl", 2010;
$ENV{PORTAGE_INTERNAL_CALL} = 1;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 LOGFILE(S)

  Present timing statistics from of a cow.sh log in tabular format.

  Use cow-timing-full.pl to get more details.
  Use canoe-timing-stats.pl to get decoder load vs translation time.

";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   debug       => \my $debug,
) or usage;

sub processlog($$) {
   my $logfile = shift;
   my $displayname = shift;
   open IN, "cow-timing-full.pl $logfile |"
      or die "Can't call cow-timing-full.pl: $!\n";
   my ($canoe, $uniq, $cat, $rtrain) = ("","","","");
   my $header_printed = 0;
   while (<IN>) {
      if ( /^   (.*):\tWALL TIME: (\d+)s/ ) {
         my $step = $1;
         my $time = $2;
         SWITCH: for ($step) {
            /^canoe/         && do { $canoe = $time; last SWITCH; };
            /^Uniq/          && do { $uniq = $time; last SWITCH; };
            /^Cat/           && do { $cat = $time; last SWITCH; };
            /^rescore_train/ && do {
               if ( !$header_printed ) {
                  print "\tdecode\tuniq n\tcat n\trescore_train (all times in seconds)\n";
                  $header_printed = 1;
               }

               $rtrain = $time; 
               print "\t$canoe\t$uniq\t$cat\t$rtrain\n";
               ($canoe, $uniq, $cat, $rtrain) = ("","","","");
               last SWITCH;
            };
            #default - unknown step - display it on its own line.
            print "   $step: ${time}s\n";
         }
      }
   }
   # Print the last line - usually partial.  Will be all blank if empty.
   print "\t$canoe\t$uniq\t$cat\t$rtrain\n";

   close IN or warn $! ? "Error closing cow-timing-full.pl pipe: $!\n"
                       : "Exit status $? from cow-timing-full.pl pipe.\n";
}

if ( @ARGV < 1 ) {
   processlog("-", "");
} else {
   #print "ARGV @ARGV\n";
   foreach my $log (@ARGV) {
      print "$log:\n";
      processlog($log, $log);
      #system("time-mem -m $log -T $log | tail -1");
   }
}

