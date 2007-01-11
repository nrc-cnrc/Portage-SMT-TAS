#!/usr/bin/perl -w

# 29_rat_test.pl - test a rescoring model using rat.sh, which combines
#                  generating the n-best list, all the features, and doing the
#                  actual translation and evaluation
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, 2006, Conseil national de recherches du Canada / National Research Council of Canada

use strict;

my $corp0 = "$ENV{PORTAGE}/test-suite/regress-small-voc";
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
my $wfr   = "${work}/wk_${fr}";
my $wfr_r = "${wfr}/cow";
my $wfr_t = "../rat_train"; # relative to $wfr_u
my $wfr_u = "${wfr}/rat_test";

mkdir $wfr_u;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

rat.sh -n 3 trans                       \\
    -f canoe.ini                        \\
    -K 200 -n 3                         \\
    $wfr_t/rescoring_model              \\
    ${corp}/realtest2000.${fr}.lowercase \\
    ${corp}/realtest2000.en.lowercase   \\
    &> log.rat_test
END

open( J1, "> ${wfr_u}/${fr}_29_rat_test" );
print J1 $job1;
close( J1 );

if ( -r "${wfr_r}/canoe.ini.cow" ) {
    print "Using canoe.ini.cow found in ${wfr_r}\n";
    system( "cp ${wfr_r}/canoe.ini.cow ${wfr_u}/canoe.ini" );
} else {
    print "No canoe.ini.cow found in ${wfr_r}.\n";
    print "Run 10_rescore.pl before this script!\n";
    exit 1;
}

system( "cd ${wfr_u}; bash ${fr}_29_rat_test" );

};


