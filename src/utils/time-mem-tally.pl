#!/usr/bin/env perl

# @file time-mem-tally.pl
# @brief Tally and summarize results from several time-mem runs.
#
# @author Eric Joanis, with enhancements by Darlene Stewart
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

  -dir|-no-dir   Turn on/off calculating subtallies by directories [-dir]
  -dhms|-no-dhms Turn on/off conversion from seconds to DDdHHhMMmSSs [-dhms]
  -ignore|-no-ignore Turn on/off ignoring of Master-Wall-Time [-no-ignore]
  -m(eta) NAME   Label the tally line with NAME, e.g., for hierarchical reports.
";
   exit 1;
}

use Getopt::Long;
my $verbose = 1;
my $dir = 1;
my $dhms = 1;
my $ignore_master_wall_time = 0;
GetOptions(
   help         => sub { usage },
   verbose      => sub { ++$verbose },
   quiet        => sub { $verbose = 0 },
   debug        => \my $debug,
   dir          => sub { $dir = 1 },
   "no-dir"     => sub { $dir = 0 },
   "dhms"       => sub { $dhms = 1 },
   "no-dhms"    => sub { $dhms = 0 },
   "ignore"       => sub { $ignore_master_wall_time = 1 },
   "no-ignore"    => sub { $ignore_master_wall_time = 0 },
   "meta=s"     => \my $meta,
) or usage;


sub format_time_mem($$$$$$$$) {
   my ($name, $wall, $cpu, $wait, $vss, $rss, $uss, $pcpu) = @_;

   my $wt_fmt = $dhms ? "%s" : "%ds";
   my $ct_fmt = $dhms ? "%s" : "%.3fs";
   my $out_fmt = "%s\tWALL TIME: " . $wt_fmt . "\tCPU TIME: " . $ct_fmt . "%s\tVSZ: %.3fG\tRSS: %.3fG%s\n";
   my $out_fmt_uss = "\tUSS: %.3fG+";
   my $out_fmt_pcpu = "\tPCPU: %.1f%%";
   
   my $cpu_plus_wait = $cpu + $wait;
   return sprintf($out_fmt, $name, $dhms ? seconds2DHMS($wall) : $wall, 
                  $dhms ? seconds2DHMS($cpu) : $cpu, 
                  $pcpu && $cpu_plus_wait ? sprintf($out_fmt_pcpu, $cpu / $cpu_plus_wait) : "", 
                  $vss, $rss, $uss ? sprintf($out_fmt_uss, $uss) : "");
}


my $indent = "   ";
my $prevdir;
my %dirs;
my ($total_wall_time, $total_cpu_time, $total_cpu_wait_time, $max_vmem, $max_ram, $max_uss) = (0,0,0,0,0,0);
my $total_do_pcpu = 1;
foreach my $file (@ARGV) { 
   if ( $file eq "-" ) { usage "Can't use - as input file.\n"; }
   zopen(*FILE, $file) or die "Can't read $file: $!\n";
   my ($wall_time, $cpu_time, $cpu_wait_time, $vmem, $ram, $uss) = (0,0,0,0,0,0);
   my $output_line = "";
   my $do_pcpu = 1;
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
      next unless /Single-job-total:|RP-Totals:|run_cmd rc=|Master-Wall-Time/;

      $debug and print "{> $_";

      # RP-Totals: Wall time 21s CPU time 40.03s Max VMEM 0.076G Max RAM 0.007G
      # RP-Totals: Wall time 85s CPU time 980.644s Max VMEM 3.426G Max RAM 1.726G Max USS 1.247G+ Avg PCPU 42.9%
      # RP-Totals: Wall time 130s CPU time 0.29s CPU Wait time 600.37s PCPU .04% Max VMEM 0.041G Max RAM 0.007G Max USS 0.004G+ Avg PCPU 0.0%
      if ( /RP-Totals: Wall time ([0-9.]+)s CPU time ([0-9.]+)s/ ) {
         $debug and print "{Wall time> $1\n{CPU> $2\n";
         $wall_time += $1;
         $cpu_time += $2;
         if ( /CPU Wait time ([0-9.]+)s / ) {
            $debug and print "{CPU Wait time> $1\n";
            $cpu_wait_time += $1;
         } else {
            $do_pcpu = 0;
         }
         if ( /Max VMEM ([0-9.]+)G Max RAM ([0-9.]+)G/ ) {
            $debug and print "{VMEM> $1\n{RAM> $2\n";
            $vmem = $1 if $1 > $vmem;
            $ram = $2 if $2 > $ram;
         }
         if ( /Max USS ([0-9.]+)G/ ) {
            $debug and print "{USS> $1\n";
            $uss = $1 if $1 > $uss;
         }
      }

      # Single-job-total: Real 0.092s User 0.028s Sys 0.000s PCPU 30.45%
      if ( /Single-job-total: Real ([0-9.dhm]+s) User ([0-9.dhm]+s) Sys ([0-9.dhm]+s)/ ) {
         $debug and print "{Real> $1\n{User> $2\n{Sys> $3\n";
         my $wall_secs = DHMSString2Seconds($1);
         $wall_time += $wall_secs;
         my $cpu_secs = DHMSString2Seconds($2) + DHMSString2Seconds($3);
         $cpu_time += $cpu_secs;
         $cpu_wait_time += $wall_secs - $cpu_secs;
      }

      # run_cmd rc=0 Max VMEM 0.071G Max RAM 0.009G
      if ( /run_cmd rc=[0-9]+ Max VMEM ([0-9.]+)G Max RAM ([0-9.]+)G/ ) {
         $debug and print "{VMEM> $1\n{RAM> $2\n";
         $vmem = $1 if $1 > $vmem;
         $ram = $2 if $2 > $ram;
      }

      # Find the overall wall time.
      if ( /Master-Wall-Time ([0-9.]+)s/ ) {
         $debug and print "{Master-Wall-Time> $1\n";
         # Master wall time was calculated for us, override previous calculations
         $wall_time = $1 unless $ignore_master_wall_time;
      }
   }
   close FILE or warn "Error closing file $file: $!\n";

   $total_wall_time += $wall_time;
   $total_cpu_time += $cpu_time;
   $total_cpu_wait_time += $cpu_wait_time;
   $max_vmem = $vmem if $vmem > $max_vmem;
   $max_ram = $ram if $ram > $max_ram;
   $max_uss = $uss if $uss > $max_uss;
   $total_do_pcpu &&= $do_pcpu;

   if ( $dir ) {
      my $dirname = $file;
      $dirname =~ s#/[^/]*$## or $dirname = ".";
      my $partialdir = $dirname;
      do {
         if ( exists $dirs{$partialdir} ) {
            $dirs{$partialdir}[0] += $wall_time;
            $dirs{$partialdir}[1] += $cpu_time;
            $dirs{$partialdir}[2] += $cpu_wait_time;
            $dirs{$partialdir}[3] = $vmem if $vmem > $dirs{$partialdir}[3];
            $dirs{$partialdir}[4] = $ram if $ram > $dirs{$partialdir}[4];
            $dirs{$partialdir}[5] = $uss if $uss > $dirs{$partialdir}[5];
            $dirs{$partialdir}[6] &&= $do_pcpu;
         } else {
            $dirs{$partialdir} = [$wall_time, $cpu_time, $cpu_wait_time, $vmem, $ram, $uss, $do_pcpu];
         }
      } while ($partialdir =~ s#/[^/]*$##);

      if ( $prevdir and $prevdir ne $dirname ) {
         while ( 1 ) {
            last if $dirname =~ m#\Q$prevdir\E(/|$)#;
            while ( $prevdir =~ m#/#g ) { print $indent; }
            print $indent;
            my $basename = (($prevdir =~ m#([^/]+)$#) ? $1 : $prevdir);
            print format_time_mem($basename, $dirs{$prevdir}[0], 
                                  $dirs{$prevdir}[1], $dirs{$prevdir}[2],
                                  $dirs{$prevdir}[3], $dirs{$prevdir}[4],
                                  $dirs{$prevdir}[5], $dirs{$prevdir}[6]);
            last unless $prevdir =~ s#/[^/]*$##; 
         }
      }
      
      $prevdir = $dirname;
   }

   if ( $output_line eq "" ) {
      my $basename = (($dir and $file =~ m#([^/]+)$#) ? $1 : $file);
      $output_line = format_time_mem($basename, $wall_time, $cpu_time, 
                                     $cpu_wait_time, $vmem, $ram, $uss, $do_pcpu);
   }
   if ( $dir ) {
      while ( $file =~ m#/#g ) { print $indent; }
   }
   print $indent, $output_line;
}

if ( $prevdir ) {
   do {
      while ( $prevdir =~ m#/#g ) { print $indent; }
      print $indent;
      my $basename = (($prevdir =~ m#([^/]+)$#) ? $1 : $prevdir);
      print format_time_mem($basename, $dirs{$prevdir}[0], $dirs{$prevdir}[1],
                            $dirs{$prevdir}[2], $dirs{$prevdir}[3], $dirs{$prevdir}[4],
                            $dirs{$prevdir}[5], $dirs{$prevdir}[6]);

   } while ($prevdir =~ s#/[^/]*$##); 
}

#foreach my $key ( sort keys %dirs ) {
#   print ">: $key:TIME-MEM\tWALL TIME: $dirs{$key}[0]s\tCPU TIME: $dirs{$key}[0]s\tVSZ: $dirs{$key}[0]G\tRSS: $dirs{$key}[0]G\n";
#}

if ( @ARGV > 0 ) {
   $meta and print "$meta:";
   print format_time_mem("TIME-MEM", $total_wall_time, $total_cpu_time, $total_cpu_wait_time,
                                     $max_vmem, $max_ram, $max_uss, $total_do_pcpu);
}
