#!/usr/bin/env perl

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
Usage: $0 [options]  CPT_IN  DM_IN  DM_FILT_OUT

  Filters a distortion model based on the entries in a conditional phrase
  table.

  CPT_IN      required conditional phrase table use to prune source ||| target
              entries in the distortion model.
  DM_IN       required distortion model to filter.
  DM_FILT_OUT output file to contain the filtered distortion model.

Options:

  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -d(ebug)      print debugging information
";
   exit @_ ? 1 : 0;
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
) or usage "Error: Invalid option(s).";

# Make sure that the user provided us with at least a conditional phrase table.
0 == @ARGV and usage "Missing parameter(s): you must provide at least the CPT.";

my $CPT   = shift or die "Error: Missing your Conditional Phrase Table!";  # What is the conditional phrase table.
my $DM    = shift or die "Error: Missing your Lexicalized Distortion Model!";  # What is the distortion model to filter.
my $FILT  = shift or die "Error: Missing your output filtered filename!";  # Where should we send the output.

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";

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

$DM !~ /.tpldm$/ or die "Error: Can't filter tightly packed distortion models (TPLDMs).\n";

if (system("zcat -f $CPT | LC_ALL=C sort -c >& /dev/null") != 0) {
   print STDERR "$CPT is NOT sorted\n" if ($debug || $verbose > 1);
   open(CPT, "zcat -f $CPT | LC_ALL=C sort |") or die "Error: Can't open conditional phrase table ($CPT) for reading: $!\n";
}
else {
   print STDERR "$CPT is sorted\n" if ($debug || $verbose > 1);
   open(CPT, "zcat -f $CPT |") or die "Error: Can't open conditional phrase table ($CPT) for reading: $!\n";
}

if (system("zcat -f $DM  | LC_ALL=C sort -c >& /dev/null") != 0) {
   print STDERR "$DM is NOT sorted\n" if ($debug || $verbose > 1);
   open(DM, "zcat -f $DM  | LC_ALL=C sort |") or die "Error: Can't open distortion model ($DM) for reading: $!\n";
}
else {
   print STDERR "$DM is sorted\n" if ($debug || $verbose > 1);
   open(DM, "zcat -f $DM  |") or die "Error: Can't open distortion model ($DM) for reading: $!\n";
}

if ($FILT =~ /\.gz$/) {
   open(FILT, "| gzip > $FILT") or die "Error: Can't open output ($FILT) for writing: $!\n";
}
else {
   open(FILT, ">$FILT") or die "Error: Can't open output ($FILT) for writing: $!\n";
}

my $cpt = "";  # Will ultimately contain the source ||| target ||| of the conditional phrase table.
my $dm_entry = "";  # This is the original entry from the distortion model for easier output.
my $dm = "";  # Will ultimately contain the source ||| target ||| of the distortion model.

#my $prev_cpt = "";
#my $prev_dm = "";

# This loop drives the algorithm by going through all conditional phrase table entries.
mainloop: while (defined ($cpt = <CPT>)) {
   # Remove probs and unwanted newline.
   #chomp $cpt;
   # Keep only the first two columns.
   $cpt = join(" ||| ", (split(/ \|\|\| /, $cpt))[0,1]) . " ||| ";

   #if ( $prev_cpt ge $cpt ) { warn "Warning: CPT $prev_cpt >= $cpt"; }

   # TODO: should I also remove the spaces to make the cpt and dm more likely
   # to be equal and white space independent.

   # This loop processes all distortion model entries.
   do {
      # Do we need a new entry from the distortion model?
      #$prev_dm = $dm;
      $dm_entry = <DM> unless ($dm ge $cpt);

      # Make sure we are not a the end of the distortion model.
      unless (defined $dm_entry) {
         $dm = "";
         $dm_entry = "";
         last mainloop;
      }

      # Remove probs and unwanted newline.
      #chomp $dm_entry;
      $dm = $dm_entry;
      # Keep only the first two columns.
      $dm = join(" ||| ", (split(/ \|\|\| /, $dm))[0,1]) . " ||| ";

      #if ( $prev_dm gt $dm ) { warn "Warning: LDM $prev_dm > $dm"; }

      print STDERR "\tCPT: $cpt\n\tDM: $dm\n\n" if ($debug);

      # This distortion model entry is part of the conditional phrase table, keep it.
      print FILT $dm_entry if ($dm eq $cpt);
   } while ($dm lt $cpt );

   #$prev_cpt = $cpt;
}

close(CPT);
close(DM);
close(FILT);

# Each Lexicalized Distortion Model should be accompanied of a bkoff file.
# Let's create the bkoff file for the filtered model.
unless ($FILT eq "-") {
   $DM    =~ s/(\.gz)?$/.bkoff/;
   $FILT  =~ s/(\.gz)?$/.bkoff/;
   my $cmd = "cp $DM $FILT";
   print STDERR "$cmd\n" if ($debug);
   system("$cmd") == 0 or die "Error: Problem creating the bkoff file ($?)."
}
else {
   warn "Warning: You need to copy the proper bkoff model for $FILT\n";
}

