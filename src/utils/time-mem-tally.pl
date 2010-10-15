#!/usr/bin/perl
# $Id$

# @file time-mem-tally.pl
# @brief Tally and summarize results from several time-mem runs.
#
# @author Eric Joanis
#
# This script is a rewrite of time-mem -T in Perl, to make it faster.
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
      unshift @INC, "$bin_path/../utils", $bin_path;
   }
}
use portage_utils;
printCopyright "time-mem-tally.pl", 2010;
$ENV{PORTAGE_INTERNAL_CALL} = 1;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [Options] {time-mem_log_files}

  Tally and summarize results from several time-mem runs.
  Intended to replace time-mem -T.

Options:

  -dir|-no-dir  Turns on or off calculating subtallies by directories [-dir]
  -m(eta) NAME  Label the tally line with NAME, e.g., for hierarchical reports.
";
   exit 1;
}

use Getopt::Long;
my $verbose = 1;
my $dir = 1;
GetOptions(
   help         => sub { usage },
   verbose      => sub { ++$verbose },
   quiet        => sub { $verbose = 0 },
   debug        => \my $debug,
   dir          => sub { $dir = 1 },
   "no-dir"     => sub { $dir = 0 },
   "meta=s"     => \my $meta,
) or usage;

my $indent = "   ";
my $prevdir;
my %dirs;
my ($total_wall_time, $total_cpu_time, $max_vmem, $max_ram) = (0,0,0,0);
foreach my $file (@ARGV) { 
   if ( $file eq "-" ) { usage "Can't use - as input file.\n"; }
   zopen(*FILE, $file) or die "Can't read $file: $!\n";
   my ($wall_time, $cpu_time, $vmem, $ram) = (0,0,0,0);
   my $output_line = "";
   while (<FILE>) {
      if ( /^(?:P-RES-MON|TIME-MEM)	WALL TIME: ([0-9.])s	CPU TIME: ([0-9.])s	VSZ: ([0-9.])G	RSS: ([0-9.]+)G/ ) {
         $wall_time = $1;
         $cpu_time = $2;
         $vmem = $3;
         $ram = $4;
         $output_line = $_;
         $output_line =~ s/^(?:P-RES-MON|TIME-MEM)/$file/;
         last;
      }
      next unless /Single-job-total:|RP-Totals:|run_cmd rc=0|Master-Wall-Time/;

      $debug and print "{> $_";

      # RP-Totals: Wall time 21s CPU time 40.03s Max VMEM 0.076G Max RAM 0.007G
      if ( /RP-Totals: Wall time ([0-9.]+)s CPU time ([0-9.]+)s Max VMEM ([0-9.]+)G Max RAM ([0-9.]+)G/ ) {
         $debug and print "{Wall time> $1\n{CPU> $2\n{VMEM> $3\n{RAM> $4 \n";
         $wall_time += $1;
         $cpu_time += $2;
         $vmem = $3 if $3 > $vmem;
         $ram = $4 if $4 > $ram;
      }

      # Single-job-total: Real 0.092s User 0.028s Sys 0.000s PCPU 30.45%
      if ( /Single-job-total: Real ([0-9.dhm]+s) User ([0-9.dhm]+s) Sys ([0-9.dhm]+s)/ ) {
         $debug and print "{Real> $1\n{User> $2\n{Sys> $3\n";
         $wall_time += DHMSString2Seconds($1);
         $cpu_time += (DHMSString2Seconds($2) + DHMSString2Seconds($3));
      }

      # run_cmd rc=0 Max VMEM 0.071G Max RAM 0.009G
      if ( /run_cmd rc=0 Max VMEM ([0-9.]+)G Max RAM ([0-9.]+)G/ ) {
         $debug and print "{VMEM> $1\n{RAM> $2\n";
         $vmem = $1 if $1 > $vmem;
         $ram = $2 if $2 > $ram;
      }

      # Find the overall wall time.
      if ( /Master-Wall-Time ([0-9.]+)s/ ) {
         $debug and print "{Master-Wall-Time> $1\n";
         # Master wall time was calculated for us, override previous calculations
         $wall_time = $1;
      }

   }
   close FILE or warn "Error closing file $file: $!\n";

   if ( $output_line eq "" ) {
      my $basename = (($dir and $file =~ m#([^/]+)$#) ? $1 : $file);
      $output_line = "$basename:TIME-MEM\tWALL TIME: ${wall_time}s\tCPU TIME: ${cpu_time}s\tVSZ: ${vmem}G\tRSS: ${ram}G\n";
   }

   $total_wall_time += $wall_time;
   $total_cpu_time += $cpu_time;
   $max_vmem = $vmem if $vmem > $max_vmem;
   $max_ram = $ram if $ram > $max_ram;

   if ( $dir ) {
      my $dirname = $file;
      $dirname =~ s#/[^/]*$## or $dirname = ".";
      my $partialdir = $dirname;
      do {
         if ( exists $dirs{$partialdir} ) {
            $dirs{$partialdir}[0] += $wall_time;
            $dirs{$partialdir}[1] += $cpu_time;
            $dirs{$partialdir}[2] = $vmem if $vmem > $dirs{$partialdir}[2];
            $dirs{$partialdir}[3] = $ram if $ram > $dirs{$partialdir}[3];
         } else {
            $dirs{$partialdir} = [$wall_time, $cpu_time, $vmem, $ram];
         }
      }
      while ($partialdir =~ s#/[^/]*$##);

      if ( $prevdir and $prevdir ne $dirname ) {
         while ( 1 ) {
            last if $dirname =~ m#\Q$prevdir\E(/|$)#;
            while ( $prevdir =~ m#/#g ) { print $indent; }
            my $basename = (($prevdir =~ m#([^/]+)$#) ? $1 : $prevdir);
            print "$indent$basename:TIME-MEM\tWALL TIME: $dirs{$prevdir}[0]s\tCPU TIME: $dirs{$prevdir}[1]s\tVSZ: $dirs{$prevdir}[2]G\tRSS: $dirs{$prevdir}[3]G\n";

            last unless $prevdir =~ s#/[^/]*$##; 
         }
      }
      
      $prevdir = $dirname;
   }

   if ( $dir ) {
      while ( $file =~ m#/#g ) { print $indent; }
   }
   print $indent, $output_line;
}

if ( $prevdir ) {
   do {
      while ( $prevdir =~ m#/#g ) { print $indent; }
      my $basename = (($prevdir =~ m#([^/]+)$#) ? $1 : $prevdir);
      print "$indent$basename:TIME-MEM\tWALL TIME: $dirs{$prevdir}[0]s\tCPU TIME: $dirs{$prevdir}[1]s\tVSZ: $dirs{$prevdir}[2]G\tRSS: $dirs{$prevdir}[3]G\n";

   } while ($prevdir =~ s#/[^/]*$##); 
}

#foreach my $key ( sort keys %dirs ) {
#   print ">: $key:TIME-MEM\tWALL TIME: $dirs{$key}[0]s\tCPU TIME: $dirs{$key}[0]s\tVSZ: $dirs{$key}[0]G\tRSS: $dirs{$key}[0]G\n";
#}

if ( @ARGV > 0 ) {
   $meta and print "$meta:";
   print "TIME-MEM\tWALL TIME: ${total_wall_time}s\tCPU TIME: ${total_cpu_time}s\tVSZ: ${max_vmem}G\tRSS: ${max_ram}G\n";
}
