#!/usr/bin/env perl

# @file analyze-plive-logs.pl
# @brief Analyze the temp files created by PortageLive and tally statistics
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

  Analyze the temp files created by PortageLive and tally statistics

Options:

  -newer TFILE  Count only files more recent than TFILE

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
   "newer=s"   => \my $newer,
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
) or usage;

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

-d $PLiveTempDir ||
   die "$PLiveTempDir is not a directory.\nPlease edit $0 and set PLiveTempDir to the directory where PortageLive creates its temporary files, typically /var/www/html/plive.\n";

my $find_cmd = "find $PLiveTempDir -name trace";
if (defined $newer) {
   -r $newer or die "Cannot read time stamp file $newer: $!\n";
   print STDERR "Counting only data files modified or created since ",
                `stat -c\%y $newer | sed 's/\\.00*//'`;
   $find_cmd .= " -newer $newer";
}

open IN, "$find_cmd |"
   or die "Can't open pipe to read $PLiveTempDir\n";

my %contexts;
while (<IN>) {
   my $tracefile = $_;
   chomp $tracefile;
   my $tracedir = $tracefile;
   $tracedir =~ s/\/trace$//;
   open TRACE, $tracefile or die "Can't open $tracefile: $!\n";
   while (<TRACE>) {
      if (/ -f=.*?models\/(.*?)\/canoe.ini.cow/) {
         my $context = $1;
         ++$contexts{$context}{n};
         my $Q_pre = "$tracedir/Q.pre";
         if (-f $Q_pre) {
            no warnings;
            $contexts{$context}{lines} += `wc -l $tracedir/Q.pre`;
            $contexts{$context}{words} += `wc -w $tracedir/Q.pre`;
         } else {
            $contexts{$context}{lines} += 0;
            $contexts{$context}{words} += 0;
         }
         last;
      }
   }
   close TRACE;
}

close IN or die "Error closing pipe to read $PLiveTempDir - output is probably wrong.\n";

if (!%contexts) {
   print STDERR "No data found.\n";
} else {
   open OUT, "| expand-auto.pl" or die "Cannot find expand-auto.pl - was Portage installed?\n";

   print OUT "Context\tDocs\tLines\tSrc Words\n";
   foreach my $key (sort keys %contexts) {
      print OUT "$key\t$contexts{$key}{n}\t$contexts{$key}{lines}\t$contexts{$key}{words}\n";
   }

   close OUT or die "Error closing output pipe, report probably not complete: $!\n";
}
