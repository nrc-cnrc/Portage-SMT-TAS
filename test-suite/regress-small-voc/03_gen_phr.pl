#!/usr/bin/perl -w

# 03_gen_phr.pl - generate phrase based translation tables
#
# PROGRAMMER: Howard Johnson / Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, 2006, Her Majesty in Right of Canada

use strict;

my $corp = "$ENV{PORTAGE}/test-suite/regress-small-voc/lc";
my $work = `dirname $0`; chomp $work;
if ( $work =~ /^[^\/]/ ) {
  my $cwd = `pwd`;
  chomp $cwd;
  $work = "$cwd/$work";
}

my @fr = (
  'fr',
#  'de',
#  'es',
#  'fi',
);
if ( @ARGV ) { @fr = @ARGV }

foreach my $fr ( @fr ) {

my $wfr = "${work}/wk_${fr}";


my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

# This script generates many redundant phrase tables, for testing purposes.

gen_phrase_tables -v -w1 -m8 -1 ${fr} -2 en -ibm 2 -z \\
  -s "GTSmoother"               \\
  -tmtext                       \\
  -o phrases-GT                 \\
  ibm2.en_given_${fr}.gz        \\
  ibm2.${fr}_given_en.gz        \\
  ${corp}/europarl.${fr}-en.${fr}.lowercase \\
  ${corp}/europarl.${fr}-en.en.lowercase    \\
  &> log.gen_gt_phrase_tables

gen_phrase_tables -v -w1 -m8 -1 ${fr} -2 en -ibm 2 -z \\
  -s "KNSmoother"               \\
  -tmtext                       \\
  -o phrases-KN                 \\
  ibm2.en_given_${fr}.gz        \\
  ibm2.${fr}_given_en.gz        \\
  ${corp}/europarl.${fr}-en.${fr}.lowercase \\
  ${corp}/europarl.${fr}-en.en.lowercase    \\
  &> log.gen_kn_phrase_tables

gen_phrase_tables -v -w1 -m8 -1 ${fr} -2 en -ibm 2 -z \\
  -s GTSmoother -s KNSmoother   \\
  -multipr fwd                  \\
  -o phrases-GT-KN              \\
  ibm2.en_given_${fr}.gz        \\
  ibm2.${fr}_given_en.gz        \\
  ${corp}/europarl.${fr}-en.${fr}.lowercase \\
  ${corp}/europarl.${fr}-en.en.lowercase    \\
  &> log.gen_multi_prob_phrase_tables

gen_phrase_tables -v -w1 -m8 -1 ${fr} -2 en -ibm 2 -z \\
  -s "LeaveOneOut 1"            \\
  -tmtext                       \\
  -o phrases-l1o                \\
  ibm2.en_given_${fr}.gz        \\
  ibm2.${fr}_given_en.gz        \\
  ${corp}/europarl.${fr}-en.${fr}.lowercase \\
  ${corp}/europarl.${fr}-en.en.lowercase    \\
  &> log.gen_l1o_tables
END

open( J1, "> ${wfr}/${fr}_03_gphr" );
print J1 $job1;
close( J1 );

print "Running gen_phrase_tables for $fr...\n";
my $rc = system( "cd ${wfr}; bash ${fr}_03_gphr" );
print "RC = $rc\n";

};
