#!/usr/bin/perl -w

# 02_train_ibm.pl - train IBM translation tables
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

print "CORP $corp\nWORK $work\n";

my @fr = (
  'fr',
#  'de',
#  'es',
#  'fi',
);
if ( @ARGV ) { @fr = @ARGV }

foreach my $fr ( @fr ) {

my $wfr = "${work}/wk_${fr}";
print "W_LANG $wfr\n";

mkdir "${wfr}";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

train_ibm -v  -s                            \\
  ibm1.en_given_${fr}.gz                    \\
  ibm2.en_given_${fr}.gz                    \\
  ${corp}/europarl.${fr}-en.${fr}.lowercase \\
  ${corp}/europarl.${fr}-en.en.lowercase    \\
  &> log.train_ibm.en_given_${fr}
END

open( J1, "> ${wfr}/${fr}_02_tribm_f" );
print J1 $job1;
close( J1 );

print "Generating forward IBM tables for $fr...\n";
my $rc = system( "cd ${wfr}; bash ${fr}_02_tribm_f" );
print "RC = $rc\n";

my $job2 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

train_ibm -vr -s                            \\
  ibm1.${fr}_given_en.gz                    \\
  ibm2.${fr}_given_en.gz                    \\
  ${corp}/europarl.${fr}-en.${fr}.lowercase \\
  ${corp}/europarl.${fr}-en.en.lowercase    \\
  &> log.train_ibm.${fr}_given_en
END

open( J2, "> ${wfr}/${fr}_02_tribm_r" );
print J2 $job2;
close( J2 );

print "Generating backward IBM tables for $fr...\n";
$rc = system( "cd ${wfr}; bash ${fr}_02_tribm_r" );
print "RC = $rc\n";

};
