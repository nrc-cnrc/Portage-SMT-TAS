#!/usr/bin/perl -w

# 08_rcanoe.pl - generate nbest lists and features for rescore translation
#                29_rat_test.pl makes this script obsolete
#
# PROGRAMMER: Howard Johnson / Eric Joanis
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

my $wfr = "${work}/wk_${fr}";
my $wfr_r = "${wfr}/cow";
my $wfr_u = "${wfr}/rtest";

mkdir $wfr_u;
mkdir "$wfr_u/f";

# Will be run in directory $wfr_u
my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

canoe               \\
  -f canoe.ini      \\
  -ffvals           \\
  -nbest f/foo:200  \\
  < ${corp}/realtest2000.${fr}.lowercase > rcanoe.out
cat f/foo.????.200best > text_en.nbest
cat f/foo.????.200best.ffvals > ffout
rm -rf f
cut -f1 ffout > ff.distortion
cut -f2 ffout > ff.wordpenalty
cut -f3 ffout > ff.segmentation
cut -f4 ffout > ff.lm
cut -f5 ffout > ff.phrase-tm
gen_feature_values                              \\
  LengthFF xx                                   \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.LengthFF
gen_feature_values                              \\
  NgramFF ${corp0}/europarl.en.srilm            \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.NgramFF
gen_feature_values                              \\
  IBM1TgtGivenSrc ../ibm1.en_given_${fr}.gz     \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.IBM1TgtGivenSrc
gen_feature_values                              \\
  IBM1SrcGivenTgt ../ibm1.${fr}_given_en.gz     \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.IBM1SrcGivenTgt
gen_feature_values                              \\
  IBM2TgtGivenSrc ../ibm2.en_given_${fr}.gz     \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.IBM2TgtGivenSrc
gen_feature_values                              \\
  IBM2SrcGivenTgt ../ibm2.${fr}_given_en.gz     \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.IBM2SrcGivenTgt
gen_feature_values                                   \\
  IBM1WTransTgtGivenSrc ../ibm1.en_given_${fr}.gz    \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.WTransIBM1TgtGivenSrc
gen_feature_values                                   \\
  IBM1WTransSrcGivenTgt ../ibm1.${fr}_given_en.gz    \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > ff.WTransIBM1SrcGivenTgt
END

open( J1, "> ${wfr_u}/${fr}_08_rcanoe" );
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

system( "cd ${wfr_u}; bash ${fr}_08_rcanoe" );

};
