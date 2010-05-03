#!/usr/bin/perl
# $Id$
# @file canoe-timing-stats.pl
# @brief Provide timing statistics for a canoe or cow or rat run.
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
printCopyright "canoe-timing-stats.pl", 2010;
$ENV{PORTAGE_INTERNAL_CALL} = 1;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 LOGFILE(S)

  Extract timing statistics out of a log from a cow.sh, canoe,
  canoe-parallel.sh, rat.sh run, or any other scripts that wraps
  canoe or canoe-parallel.sh.

  Focuses specically on model load time vs translation time.
  Use cow-timing.pl to get detailed timing information for cow.sh.

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

sub sum(\@) {
   my $sum = 0;
   foreach (@{$_[0]}) {
      $sum += $_;
   }
   $sum;
}

my $has_high_load_ratio = 0;

sub processlog($$) {
   my $logfile = shift;
   my $displayname = shift;
   my @loadtime;
   my @translatetime;
   my @sentences;
   open LOG, $logfile or die "Can't open $logfile: $!\n";
   while (<LOG>) {
      if ( /Loaded data structures in (\d+) seconds./ ) {
         push @loadtime, $1;
      }
      if ( /Translated (\d+) sentences in (\d+) seconds./ ) {
         push @sentences, $1;
         push @translatetime, $2;
      }
   }

   my $totalloadtime = sum @loadtime;
   my $totaltranstime = sum @translatetime;
   my $totalsentences = sum @sentences;
   my $instances = scalar @translatetime;
   my $load_trans_ratio = $totalloadtime / ($totaltranstime || 1);

   if ( $displayname ) {
      print "====== $displayname ======\n";
   }
   if ( ! $instances ) {
      print "No canoe instances found\n\n";
      return;
   }
   print "individual canoe instances: $instances\n";
   printf "load time: total = ${totalloadtime}s; avg = %.1fs; per-sent = %.2fs\n",
      $totalloadtime / ($instances || 1), 
      $totalloadtime / ($totalsentences || 1);
   printf "trans time: total = ${totaltranstime}s; avg = %.1fs; per-sent = %.2fs\n",
      $totaltranstime / ($instances || 1),
      $totaltranstime / ($totalsentences || 1);
   printf "load/trans ratio = %.2f", $load_trans_ratio;
   if ( $load_trans_ratio > 10 ) { print "   is EXCEEDINGLY HIGH."; }
   elsif ( $load_trans_ratio > 5 ) { print "   is VERY HIGH."; }
   elsif ( $load_trans_ratio > 1 ) { print "   is HIGH."; }
   print "\n\n";
   if ( $load_trans_ratio > 1 ) {
      $has_high_load_ratio = 1;
   }
}

if ( @ARGV < 1 ) {
   processlog("-", "");
} elsif ( @ARGV == 1 ) {
   processlog(shift, "");
} else {
   #print "ARGV @ARGV\n";
   foreach my $log (@ARGV) {
      processlog($log, $log);
   }
   #print "ARGV @ARGV\n";
   processlog("cat @ARGV |", "Global statistics");
}

if ( $has_high_load_ratio ) {
   print "A load/trans ratio above 1 is high - consider reducing parallelism or\n";
   print "optimizing load time, e.g., via filtering or tightly packed models.\n";
}
