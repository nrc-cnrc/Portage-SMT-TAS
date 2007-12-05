#!/usr/bin/perl
# $Id$

# process-memory-usage.pl
#
# PROGRAMMER: Samuel Larkin
#
# COMMENTS:
#
# Samuel Larkin
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

use strict;
use warnings;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [-h(elp)] [-v(erbose)] SLEEP PID

  Sums the virtual memory usage and resident set size of the tree rooted in PID
  every SLEEP seconds.

Options:

  -h(elp):      print this help message

  -v(erbose):   increment the verbosity level by 1 (may be repeated)

";
   exit 1;
}

my $Gigabytes = 1024 * 1024;  # Kb to Gb
use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   "help|h"        => sub { usage },
   "verbose|v"     => sub { ++$verbose },
   "debug|d"       => \my $debug,
) or usage;

my $sleep_time = shift || die "Missing SLEEP";
my $mainpid    = shift || die "Missing PID";
#print "$sleep_time  $mainpid\n";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";


my $i = 0;
#while (++$i < 2) {
while (1) {
   sleep $sleep_time;
   my %PIDS = ();
   my $total_vsz = 0;
   my $total_rss = 0;
   foreach my $process_info (split /\n/, `ps xo ppid,pid,vsz,rss,comm`) {
      print "<V> $process_info\n" if ($verbose > 1);

      next if ($process_info =~ /PPID/);  # Skip the header

      chomp($process_info);        # Remove newline
      $process_info =~ s/^\s+//;   # Remove leading spaces
      my ($ppid, $pid, $vsz, $rss, $comm) = split(/\s+/, $process_info);

      print "<D> $ppid $pid $vsz $rss $comm\n" if(defined($debug));

      # Is this process part of the process tree
      if (exists $PIDS{$ppid} or $pid == $mainpid) {
         $PIDS{$pid} = 1;
         $total_vsz += $vsz;
         $total_rss += $rss;
      }
   }
   my $date = `date`;
   chomp($date);
   $total_vsz = $total_vsz / $Gigabytes;
   $total_rss = $total_rss / $Gigabytes;
   printf "$date Process tree for $mainpid uses vsz: %.3fG rss: %.3fG ", $total_vsz , $total_rss;
   print "Processes: " . join(" ",keys (%PIDS)) . "\n";
}
