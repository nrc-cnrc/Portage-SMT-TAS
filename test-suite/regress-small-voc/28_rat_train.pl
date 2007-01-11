#!/usr/bin/perl -w

# 28_rat_train.pl - train a rescoring model using rat.sh, which combines
#                   generating the n-best list, all the features, and doing the
#                   actual training
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
my $wfr_t = "${wfr}/rat_train";

mkdir $wfr_t;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

rat.sh -n 3 train                       \\
    -f canoe.ini                        \\
       -K 200 -n 3                      \\
    rescoring_model                     \\
    ${corp}/test2000.${fr}.lowercase    \\
    ${corp}/test2000.en.lowercase       \\
    &> log.rat_train
END

open( J1, "> ${wfr_t}/${fr}_28_rat_train" );
print J1 $job1;
close( J1 );

if ( -r "${wfr_r}/canoe.ini.cow" ) {
    print "Using canoe.ini.cow found in ${wfr_r}\n";
    system( "cp ${wfr_r}/canoe.ini.cow ${wfr_t}/canoe.ini" );
} else {
    print "No canoe.ini.cow found in ${wfr_r}.\n";
    print "Run 10_rescore.pl before this script!\n";
    exit 1;
}

my $model1 = << "END";
FileFF:ff.LengthFF
FileFF:ff.NgramFF.${corp0}/europarl.en.srilm
FileFF:ff.IBM1TgtGivenSrc.../ibm1.en_given_${fr}
FileFF:ff.IBM1SrcGivenTgt.../ibm1.${fr}_given_en
FileFF:ff.IBM2TgtGivenSrc.../ibm2.en_given_${fr}
FileFF:ff.IBM2SrcGivenTgt.../ibm2.${fr}_given_en
FileFF:ff.IBM1WTransTgtGivenSrc.../ibm1.en_given_${fr}
FileFF:ff.IBM1WTransSrcGivenTgt.../ibm1.${fr}_given_en
END

my $number_of_features = `cd ${wfr_t}; configtool nf canoe.ini`;
chomp $number_of_features;

open( MODEL, "> ${wfr_t}/rescoring_model" );
foreach my $i ( 1 .. $number_of_features ) {
    print MODEL "FileFF:ffvals,$i\n";
}
print MODEL $model1;
close( MODEL );

system( "cd ${wfr_t}; bash ${fr}_28_rat_train" );

};


