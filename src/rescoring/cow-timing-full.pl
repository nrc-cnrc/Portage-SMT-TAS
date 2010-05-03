#!/usr/bin/perl
# $Id$
# @file cow-timing-full.pl
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
printCopyright "cow-timing-full.pl", 2010;
$ENV{PORTAGE_INTERNAL_CALL} = 1;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 LOGFILE(S)

  Extract timing statistics out of a cow.sh log.

  Use cow-timing.pl to get this information in a tabular format.
  Use canoe-timing-stats.pl to get decoder load vs translation time.

Options:

  -pretty  Pretty print the output.  Equivalent to:
              $0 log.cow | second-to-hms.pl | expand-auto.pl

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
   pretty      => \my $pretty,
   debug       => \my $debug,
) or usage;

if ( $pretty ) {
   open OUT, "| second-to-hms.pl | expand-auto.pl"
      or die "Can't open to pretty printing pipe: $!\n"
} else {
   open OUT, ">-" or die "Can't open STDOUT for writing: $!\n";
}

sub sum(\@) {
   my $sum = 0;
   foreach (@{$_[0]}) {
      $sum += $_;
   }
   $sum;
}

sub parsetime($) {
   my $walltime = 0;
   my $cputime = 0;
   if ( $_[0] =~ /Real (\d+)(?:\.\d*)?s User (\d+)(?:\.\d*)?s Sys (\d+)(?:\.\d*)?s/ ) {
      $walltime = $1 . "s";
      $cputime = ($2 + $3) . "s";
   }
   return ($walltime, $cputime);
}

sub parsemem($) {
   my $ram = "";
   my $vmem = "";
   if ( /Max VMEM ([0-9.]+G) Max RAM ([0-9.]+G)/ ) {
      $ram = $1;
      $vmem = $2;
   }
   return ($ram, $vmem);
}

# parse_run_cmd_output($prev_line, $next_line, $command);
sub parse_run_cmd_output($$$) {
   my ($walltime, $cputime) = parsetime $_[0];
   my ($ram, $vmem) = parsemem $_[1];
   print OUT "   $_[2]:\tWALL TIME: $walltime\tCPU TIME: $cputime\tVSZ: $vmem\tRSS: $ram\n";
}

sub processlog($$) {
   my $logfile = shift;
   my $displayname = shift;
   my @loadtime;
   my @translatetime;
   my @sentences;
   open LOG, $logfile or die "Can't open $logfile: $!\n";
   my $prev_line = "";
   while (<LOG>) {
      # Non-parallel programs
      if ( /run_cmd finished \(rc=\d+\): ([^ ]+)/ ) {
         $_ = <LOG>;
         parse_run_cmd_output $prev_line, $_, $1;
      }
      # Canoe-parallel produces an RP-Total line
      if ( /RP-Totals: Wall time (\d+)(?:\.\d*)?s CPU time (\d+)(?:\.\d*)?s Max VMEM ([0-9.]+G) Max RAM ([0-9.]+G)/ ) {
         print OUT "   canoe-parallel:\tWALL TIME: $1s\tCPU TIME: $2s\tVSZ: $3\tRSS: $4\n";
      }
      # Producing n-best lists - running append-uniq.pl for each dev sentence
      if ( /^Producing n-best lists/ ) {
         while (<LOG>) {
            last if /Single-job-total/;
         }
         parse_run_cmd_output($_, "", "Uniq n-bests");
      }
      # Appending all n-best lists together
      if ( /^Preparing to run rescore_train/ ) {
         while (<LOG>) {
            last if /Single-job-total/;
         }
         parse_run_cmd_output($_, "", "Cat n-bests");
      }
      
      $prev_line = $_;
   }
}

if ( @ARGV < 1 ) {
   processlog("-", "");
} else {
   #print "ARGV @ARGV\n";
   foreach my $log (@ARGV) {
      print OUT "$log:\n";
      processlog($log, $log);
      #system("time-mem -m $log -T $log | tail -1");
   }
}

close OUT or warn $! ? "Error closing pretty-print pipe: $!"
                     : "Exit status $? from pretty-print pipe.";

