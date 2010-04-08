#!/usr/bin/perl
# $Id$

# @file filter-distortion-model.pl
# @brief Filter a distortion model based on a conditional phrase table.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

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
printCopyright("filter-distortion-model.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use strict;
use warnings;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] CPT_IN [DM_IN [DM_FILT_OUT]]

  Filters a distortion model based on the entries in a conditional phrase
  table.

  CPT_IN      required conditional phrase table use to prune source ||| target
              entries in the distortion model.
  DM_IN       required distortion model to filter [-].
  DM_FILT_OUT output file to contain the filtered distortion model [-].

Options:

  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -d(ebug)      print debugging information
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
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
) or usage;

# Make sure that the user provided us with at least a conditional phrase table.
0 == @ARGV and usage "Missing parameter(s): you must provide at least the CPT.";

my $CPT   = shift;  # What is the conditional phrase table.
my $DM    = shift || "-";  # What is the distortion model to filter.
my $FILT  = shift || "-";  # Where should we send the output.

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

if ( $debug ) {
   no warnings;
   print STDERR "
   CPT         = $CPT
   DM          = $DM
   FILT        = $FILT
   verbose     = $verbose
   debug       = $debug

";
}

$DM !~ /.tpldm$/ or die "Can't filter tightly packed distortion models (TPLDMs).\n";

if (system("zcat -f $CPT | LC_ALL=C sort -c >& /dev/null") != 0) {
   print STDERR "$CPT is NOT sorted\n" if ($debug || $verbose > 1);
   open(CPT, "zcat -f $CPT | LC_ALL=C sort |") or die "Can't open conditional phrase table ($CPT) for reading: $!\n";
}
else {
   print STDERR "$CPT is sorted\n" if ($debug || $verbose > 1);
   open(CPT, "zcat -f $CPT |") or die "Can't open conditional phrase table ($CPT) for reading: $!\n";
}

if (system("zcat -f $DM  | LC_ALL=C sort -c >& /dev/null") != 0) {
   print STDERR "$DM is NOT sorted\n" if ($debug || $verbose > 1);
   open(DM, "zcat -f $DM  | LC_ALL=C sort |") or die "Can't open distortion model ($DM) for reading: $!\n";
}
else {
   print STDERR "$DM is sorted\n" if ($debug || $verbose > 1);
   open(DM, "zcat -f $DM  |") or die "Can't open distortion model ($DM) for reading: $!\n";
}

open(FILT, ">$FILT") or die "Can't open output ($FILT) for writing: $!\n";

my $cpt = "";  # Will ultimately contain the source ||| target ||| of the conditional phrase table.
my $dm_entry = "";  # This is the original entry from the distortion model for easier output.
my $dm = "";  # Will ultimately contain the source ||| target ||| of the distortion model.

# How to find the probs.
my $prob = qr/\|\|\| [^\|]+$/;

# This loop drive the algorithm by going through all conditional phrase table entries.
while ($cpt = <CPT>) {
   # Remove probs and unwanted newline.
   chomp $cpt;
   $cpt =~ s/$prob//;

   # TODO: should I also remove the spaces to make the cpt and dm more likely
   # to be equal and white space independent.

   # This loop processes all distortion model entries.
   do {
      # Do we need a new entry from the distortion model?
      $dm_entry = <DM> unless ($dm ge $cpt);

      # Make sure we are not a the end of the distortion model.
      unless (defined $dm_entry) {
         $dm = "";
         $dm_entry = "";
         next;
      }

      # Remove probs and unwanted newline.
      chomp $dm_entry;
      $dm = $dm_entry;
      $dm =~ s/$prob//;

      print STDERR "\tCPT: $cpt\n\tDM: $dm\n\n" if ($debug);

      # This distortion model entry is part of the conditional phrase table, keep it.
      print FILT "$dm_entry\n" if ($dm eq $cpt);
   } while ($dm lt $cpt );
}

close(CPT);
close(DM);
close(FILT);

