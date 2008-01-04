#!/usr/bin/perl -w

# 28_rat_train.pl - train a rescoring model using rat.sh, which combines
#                   generating the n-best list, all the features, and doing the
#                   actual training
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005 - 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005 - 2008, Her Majesty in Right of Canada

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
    -K 200 -n 3                         \\
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
LengthFF
ParMismatch
QuotMismatch:fe
Consensus
ConsensusWin
NgramFF:${corp0}/europarl.en.srilm
IBM1TgtGivenSrc:../ibm1.en_given_${fr}.gz
IBM1SrcGivenTgt:../ibm1.${fr}_given_en.gz
IBM2TgtGivenSrc:../ibm2.en_given_${fr}
IBM2SrcGivenTgt:../ibm2.${fr}_given_en
IBM1WTransTgtGivenSrc:../ibm1.en_given_${fr}.gz
IBM1WTransSrcGivenTgt:../ibm1.${fr}_given_en.gz
IBM1DeletionTgtGivenSrc:../ibm1.en_given_${fr}.gz#0.2
IBM1DeletionSrcGivenTgt:../ibm1.${fr}_given_en.gz#0.2
nbestWordPostLev:1#<ffval-wts>#<pfx>
nbestWordPostTrg:1#<ffval-wts>#<pfx>
nbestNgramPost:3#1#<ffval-wts>#<pfx>
nbestSentLenPost:1#<ffval-wts>#<pfx>
nbestWordPostSrc:1#<ffval-wts>#<pfx>
nbestPhrasePostSrc:1#<ffval-wts>#<pfx>
nbestPhrasePostTrg:1#<ffval-wts>#<pfx>
END

# Use configtool to get the list of basic decoder features
my $basic_features = `cd ${wfr_t}; configtool rescore-model:ff.vals canoe.ini`;

open( MODEL, "> ${wfr_t}/rescoring_model" );
print MODEL $basic_features;
print MODEL $model1;
close( MODEL );

system( "cd ${wfr_t}; bash ${fr}_28_rat_train" );

};


