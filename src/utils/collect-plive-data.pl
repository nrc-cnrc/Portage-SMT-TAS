#!/usr/bin/env perl

# @file collect-plive-logs.pl
# @brief Collect the temp files created by PortageLive for offline analysis
#
# @author Eric Joanis
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

###############################################################################
# BEGIN CONFIGURATION SECTION
###############################################################################

# This is the directory where PortageLive's temporary folders are all created
my $PLiveTempDir = "/var/www/html/plive";
# This file will be used so successive data collection runs don't collect the
# same contents multiple times.
my $TimeStampFile = "plive-last-collect";
# This data file drives what gets collected
my $ConfigFile = "plive-stats.txt";

###############################################################################
# END CONFIGURATION SECTION
###############################################################################


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
printCopyright 2014;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options]

  Collect the temp files created by PortageLive for offline analysis

  This script collects all temporary PortageLive working directories created
  since file $TimeStampFile was last modified (which, by default, marks
  the last time this script was called).

  The script first reads $ConfigFile (or CFILE, if -config CFILE is used)
  and collects data only from those systems with a + before the system name.
  Example CFILE:
    Context             Docs   Lines   Words in
    Hansard-HOC-en2fr   2      2       5
    +generic1.0.fr2en   1      13      103
    toy.ar2en           15     30      405
    +toy.ch2en          3      3       3
  This sample config file was produced by calling analyze-plive-data.pl and
  adding a + in front of the name of the two systems to collect, namely
  generic1.0.fr2en and toy.ch2en. With this configuration, no data will be
  collected from Hansard-HOC-en2fr or toy.ar2en.

Options:

  -all          Count all files [default: count only files more recent than
                $TimeStampFile]
  -newer TFILE  Count only files more recent than TFILE [$TimeStampFile]

  -config CFILE read CFILE to decide which systems to collect from
                [$ConfigFile]

  -h(elp)       print this help message
";
   exit 1;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   all         => \my $all,
   "newer=s"   => \my $newer,
   "config=s"  => \my $config,
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
) or usage;

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

-d $PLiveTempDir ||
   die "$PLiveTempDir is not a directory.\nPlease edit $0 and set PLiveTempDir to the directory where PortageLive creates its temporary files, typically /var/www/html/plive.\n";

if (defined $config) {
   -r $config or die "Cannot read file $config: $!\n";
   $ConfigFile = $config;
}

if (defined $newer) {
   -r $newer or die "Cannot read file $newer: $!\n";
   $TimeStampFile = $newer;
}

open CONFIG, "$ConfigFile" or die "Cannot read file $ConfigFile: $!\n";
my $context_line = <CONFIG>;
$context_line =~ /^\s*Context\s/
   or die "Invalid format in $ConfigFile: expected header lnie starting with Context\n";
my %contexts;
while (<CONFIG>) {
   my ($context, $doc, $lines, $words) = split;
   if ($context =~ /^\s*\+\s*(\S+)/) {
      $context = $1;
      print "Collecting data from context $context\n";
      $contexts{$context}{collect} = "yes";
   } elsif ($context =~ /^\s*(\S+)/) {
      $context = $1;
      print "*NOT* collecting from $context\n";
      $contexts{$context}{collect} = "no";
   } else {
      print "Ignoring ill-formatted line in $ConfigFile: $_";
   }
}

my $find_cmd = "find $PLiveTempDir -name trace";
if (!$all and -r $TimeStampFile) {
   print STDERR "Collecting data only from files modified or created since ",
                `stat -c\%y $TimeStampFile | sed 's/\\.00*//'`;
   $find_cmd .= " -newer $TimeStampFile";
}

open IN, "$find_cmd |"
   or die "Can't open pipe to read $PLiveTempDir\n";

my $date = `date +%Y%m%d-%H%M`;
chomp $date;

my @tarlists;
my ($docs, $lines, $words) = (0,0,0);
while (<IN>) {
   my $tracefile = $_;
   chomp $tracefile;
   my $tracedir = $tracefile;
   $tracedir =~ s/\/trace$//;
   open TRACE, $tracefile or die "Can't open $tracefile: $!\n";
   while (<TRACE>) {
      if (/ -f=.*?models\/(.*?)\/canoe.ini.cow/) {
         my $context = $1;
         if (!exists $contexts{$context}) {
            print "NOT collecting data from unknown context $context; add it to $ConfigFile to start collecting from this context\n";
         } elsif ($contexts{$context}{collect} eq "yes") {
            ++$contexts{$context}{n};
            ++$docs;
            my $Q_pre = "$tracedir/Q.pre";
            if (-f $Q_pre) {
               no warnings;
               my $l = `wc -l $tracedir/Q.pre`;
               $contexts{$context}{lines} += $l;
               $lines += $l;
               my $w = `wc -w $tracedir/Q.pre`;
               $contexts{$context}{words} += $w;
               $words += $w;
            } else {
               $contexts{$context}{lines} += 0;
               $contexts{$context}{words} += 0;
            }

            my $tarlist = "collect-$context-$date.list";
            -r $tarlist || push @tarlists, $tarlist;
            system("echo $tracedir >> $tarlist") == 0
               or warn "Error appending to $tarlist: $!\n";
            print "Add $tracedir to $tarlist for collection\n";
         }
         last;
      }
   }
   close TRACE;
}

for my $tarlist (@tarlists) {
   my $tarball = $tarlist;
   $tarball =~ s/list$/tgz/;
   system("tar -czf $tarball --files-from $tarlist") == 0
      or warn "Error creating tar ball $tarball: $!\n";
}

close IN or warn "Error closing pipe to read $PLiveTempDir - output is probably wrong.\n";

print "Collected From\tDocs\tLines\tSrc Words\n";
print "Total\t$docs\t$lines\t$words\n";

foreach my $key (sort keys %contexts) {
   if ($contexts{$key}{collect} eq "yes") {
      print "$key\t$contexts{$key}{n}\t$contexts{$key}{lines}\t$contexts{$key}{words}\n";
   }
}
