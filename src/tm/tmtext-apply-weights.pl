#!/usr/bin/env perl
# $Id$

# @file tmtext-apply-weights.pl 
# @brief Apply know weights to a multi-probs tm.
#
# @author Eric Joanis / Samuel Larkin
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

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
printCopyright("tmtext-apply-weights.pl", 2008);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


# Shield against log 0.
sub safe_log {
   # -18 is what's used inside canoe.
   return -18 unless ($_[0] > 0);
   return log($_[0]);
}

sub error_exit {
   local $, = "\n";
   print STDERR @_, "", "Aborting.  Use -h for help\n\n";
   exit 1;
}

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 -src <src_lang> -tgt <tgt_lang> [-o <out prefix>]
       [-config|-f <canoe.ini>]
       [-h(elp)] [-v(erbose)] [in]

  Apply fixed weights to a multi-prob phrase table and output a two-prob
  phrase table (one forward and one backward), in multi-prob format.

Options:
  -o            Specify the base output name [phrase-weight-applied]
  -src, -tgt    Specify the source and target languages 
  -f|-config    Specify a canoe config file [canoe.ini]
  -c CANOE_OUT  Specify an output of a modified canoe.ini.

  -ftm|weight-f forward weights.
  -tm|weight-t  backward weights.

  -hf           Apply hard filter.
  -shf <MODEL>  Save intermediate hard-filtered model to <MODEL>.
                Note that <MODEL> will be augmented with the suffix .FILT.
  -tmdb         Produce a TMDB instead of a multi-prob TMText
  -tppt         Produce a TPPT as well as a multi-prob TMText

  -h(elp):      print this help message
  -v(erbose):   increment the verbosity level by 1 (may be repeated)
  -d(ebug):     print debugging information

";
   exit 1;
}

use Getopt::Long;
my ($opt_tm_weights, $opt_ftm_weights);
my $name = "phrase-weight-applied";
my $canoe_ini = "canoe.ini";
my $verbose = 1;
GetOptions(
   "ftm=s"             => \$opt_ftm_weights,
   "weight-f=s"        => \$opt_ftm_weights,
   "tm=s"              => \$opt_tm_weights,
   "weight-t=s"        => \$opt_tm_weights,

   "c=s"               => \my $canoe_ini_out,
   "f=s"               => \$canoe_ini,
   "config=s"          => \$canoe_ini,
   "o=s"               => \$name,
   "src=s"             => \my $opt_src_lang,
   "tgt=s"             => \my $opt_tgt_lang,

   tmdb        => \my $tmdb,
   tppt        => \my $tppt,
   hf          => \my $do_hardFilter,
   "shf=s"     => \my $hardFilter_model,

   h           => sub { usage },
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
) or usage;

defined ($opt_src_lang) or usage("-src option mandatory");
defined ($opt_tgt_lang) or usage("-tgt option mandatory");

my $in = shift || "-";

if ($do_hardFilter or defined($hardFilter_model)) {
   die "You must provide a canoe.ini file through -f.\n" unless (defined($canoe_ini));

   my $rc = system("configtool check $canoe_ini &> /dev/null");
   error_exit "Invalid canoe.ini $canoe_ini\n" unless ($rc == 0);

   if ($hardFilter_model) {
      my $rc = system("filter_models -f $canoe_ini -no-src-grep -tm-online -c -hard-limit $hardFilter_model");
      error_exit "error hard filtering!\n" unless ($rc == 0);
      if ($hardFilter_model =~ /\.gz$/) {
         $hardFilter_model =~ s/(.+)(\.gz)$/$1.FILT$2/;
      }
      else {
         $hardFilter_model += ".FILT";
      }
      $in = "zcat -f $hardFilter_model |";
   }
   else {
      $in = "filter_models -f $canoe_ini -no-src-grep -tm-online -c -hard-limit - |";
   }
}

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

my %ini;
if ( $canoe_ini ) {
   open CANOE_INI, $canoe_ini or error_exit "Can't open $canoe_ini: $!";
   local $/ = '[';
   %ini = map { 
      s/\#.*$//mg; # remove comments
      s/\[$//; # remove the [ at the end of the "line"
      if ( /^\s*$/ ) {
         ();
      } else { 
         /^\s*\[?\s*(.*?)\s*\]\s*(.*?)\s*$/s;
      }
   } <CANOE_INI>;
   close CANOE_INI;
}

my $tm_weights;
if ( defined $opt_tm_weights ) {
   $tm_weights = $opt_tm_weights;
} elsif ( exists $ini{"weight-t"} ) {
   $tm_weights = $ini{"weight-t"};
} elsif ( exists $ini{"tm"} ) {
   $tm_weights = $ini{"tm"};
} else {
   error_exit "No weights specified";
}

my $ftm_weights;
if ( defined $opt_ftm_weights ) {
   $ftm_weights = $opt_ftm_weights;
} elsif ( exists $ini{"weight-f"} ) {
   $ftm_weights = $ini{"weight-f"};
} elsif ( exists $ini{"ftm"} ) {
   $ftm_weights = $ini{"ftm"};
} else {
   warn "Setting forward weights to backward weights";
   $ftm_weights = $tm_weights;
}

my @ftm_weights = split /:|\s+/, $ftm_weights;
foreach (@ftm_weights) {
   no warnings;
   s/^(\.[0-9]+)$/0$1/;
   error_exit "Invalid weight: $_" if $_ + 0 ne $_
}

my @tm_weights = split /:|\s+/, $tm_weights;
foreach (@tm_weights) {
   no warnings;
   s/^(\.[0-9]+)$/0$1/;
   error_exit "Invalid weight: $_" if $_ + 0 ne $_
}

die "Uneven number of weights" unless (scalar(@tm_weights) == scalar(@ftm_weights));

my $expected_prob_count = scalar(@ftm_weights) + scalar(@tm_weights);
my $fwd_offset = scalar @tm_weights;
open(IN, "$in") or die "Can't open $in for reading: $!\n";

# Preparing the output/outputs.
my $file = "$name.${opt_src_lang}2${opt_tgt_lang}.gz";
my $fwd_file = "$name.${opt_tgt_lang}_given_$opt_src_lang.gz";
my $bwd_file = "$name.${opt_src_lang}_given_$opt_tgt_lang.gz";
if ($tmdb) {
   open(FWD, "| gzip > $fwd_file") or die "Can't open $fwd_file for writing: $!\n";
   open(BWD, "| gzip > $bwd_file") or die "Can't open $bwd_file for writing: $!\n";
}
else {
   open(OUT, "| gzip > $file") or die "Can't open $file for writing: $!\n";
}

# We need to sum up the backward weights to change the canoe.ini's weights.
# If we change the backward weights to 1 then OOVs will no longer have the same
# weight inside canoe.
my $tm_weight = 0;
foreach (@tm_weights) {
   $tm_weight += $_;
}

# Obviously we need to do the same thing for forward weights.
my $ftm_weight = 0;
foreach (@ftm_weights) {
   $ftm_weight += $_;
}

print STDERR "bwd weights: @tm_weights fwd weights: @ftm_weights\n" if ($debug);
print STDERR "bwd weight: $tm_weight fwd weights: $ftm_weight\n";
while (<IN>) {
   s/^\s+//;
   s/\s+$//;
   my ($src, $tgt, $probs) = split(/\s+\|\|\|\s+/, $_, 3);
   my @probs = split(/\s+/, $probs);
   if ($debug) {
      print STDERR "src=$src:\n";
      print STDERR "tgt=$tgt:\n";
      print STDERR "weights=@tm_weights @ftm_weights\n";
      print STDERR "probs=$probs:\n";
      print STDERR ":@probs:\n";
   }
   if ($expected_prob_count != @probs) {
      print STDERR "src=$src:\n";
      print STDERR "tgt=$tgt:\n";
      print STDERR "probs=$probs:\n";
      print STDERR ":@probs:\n";
      die "Unexpected number of probs at $.: $_";
   }

   my $bwd_prob = 0;
   my $fwd_prob = 0;
   for (my $i=0; $i<@tm_weights; ++$i) {
      $bwd_prob += $tm_weights[$i] * safe_log($probs[$i]);
      $fwd_prob += $ftm_weights[$i] * safe_log($probs[$i+$fwd_offset]);
   }

   # Here we could use a weight of 1 and output $bwd_prob and $fwd_prob as
   # computed so far, but this would cause a problem with canoe, in how it
   # handles missing phrases.  When a phrase is missing from the phrase table,
   # such as is always the case with OOVs, canoe assigns log(p)=-18 to that
   # phrase, which then gets multiplied by the weight.  If we apply all the
   # weights and set the new decoder weight to 1, an OOV gets the score 1 *
   # -18.  But if we decode with the input tables, it gets the score $tm_weight
   # * -18.  Since this program should be strictly an optimization that doesn't
   # change the decoder's output, we have to keep the weight to the sum of the
   # original weights, hence the correction applied here.
   $bwd_prob = exp($bwd_prob / $tm_weight);
   $fwd_prob = exp($fwd_prob / $ftm_weight);

   if ($tmdb) {
      printf BWD "%s ||| %s ||| %.9g\n", $src, $tgt, $bwd_prob;
      printf FWD "%s ||| %s ||| %.9g\n", $tgt, $src, $fwd_prob;
   }
   else {
      printf OUT "%s ||| %s ||| %.9g %.9g\n", $src, $tgt, $bwd_prob, $fwd_prob;
   }
}
close(IN) or die "An ERROR occurred while processing \"$in\"";

$verbose and print STDERR "$file creation completed.\n";

if ($tmdb) {
   close(BWD);
   close(FWD);
}
else {
   close(OUT);
}

if ($tppt) {
   my $cmd = "time textpt2tppt.sh $file";
   $verbose and print STDERR "Running: $cmd\n";
   system($cmd);

   # In case the user asked for a modified canoe.ini with need to modify the file name.
   $file =~ s/gz$/tppt/;  # the phrase table is now .tppt.
}
elsif ($tmdb) {
   system("time tmtext2tmdb -force $bwd_file $fwd_file");
   #TODO: produce a valid canoe.ini if asked for it.
}

if (defined($canoe_ini_out)) {
   my $cmd = "configtool -c applied-weights:$file:$tm_weight:$ftm_weight $canoe_ini > $canoe_ini_out";
   $verbose and print STDERR "Running: $cmd\n";
   system($cmd);
}

