#!/usr/bin/perl -w

# 10_rat_train.pl - train a rescoring model using rat.sh, which combines
#                   generating the n-best list, all the features, and doing the
#                   actual training
#
# PROGRAMMER: Eric Joanis / Samuel Larkin
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
my $base_work = `dirname $0`; chomp $base_work;
if ( $base_work =~ /^[^\/]/ ) {
  my $cwd = `pwd`;
  chomp $cwd;
  $base_work = "$cwd/$base_work";
}

my @src_lang = (
  'fr',
#  'de',
#  'es',
#  'fi',
);
if ( @ARGV ) { @src_lang = @ARGV }

foreach my $src_lang ( @src_lang ) {
my $workdir0 = "${base_work}/wk_${src_lang}";
my $workdir = "${workdir0}/rat_train";
my $decoder_training = "${workdir0}/cow";

mkdir $workdir;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

echo -n "Training rescoring-model "
rat.sh -n 3 train                       \\
    -f canoe.ini                        \\
    -K 200 -n 3                         \\
    rescoring_model                     \\
    ${corp}/test2000.${src_lang}.lowercase    \\
    ${corp}/test2000.en.lowercase       \\
    &> log.rat_train                    \\
&& echo "OK" || echo "FAILED"
END

my $script = "${workdir}/${src_lang}_10_rat_train";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

if ( -r "${decoder_training}/canoe.ini.cow" ) {
    print "Using canoe.ini.cow found in ${decoder_training}\n";
    system( "cp ${decoder_training}/canoe.ini.cow ${workdir}/canoe.ini" );
} else {
    print "No canoe.ini.cow found in ${decoder_training}.\n";
    print "Run 05_cow.pl before this script!\n";
    exit 1;
}

my $model1 = << "END";
LengthFF
ParMismatch
QuotMismatch:fe
NgramFF:${corp0}/europarl.en.srilm
IBM1TgtGivenSrc:../ibm1.en_given_${src_lang}.gz
IBM1SrcGivenTgt:../ibm1.${src_lang}_given_en.gz
IBM2TgtGivenSrc:../ibm2.en_given_${src_lang}
IBM2SrcGivenTgt:../ibm2.${src_lang}_given_en
IBM1WTransTgtGivenSrc:../ibm1.en_given_${src_lang}.gz
IBM1WTransSrcGivenTgt:../ibm1.${src_lang}_given_en.gz
IBM1DeletionTgtGivenSrc:../ibm1.en_given_${src_lang}.gz#0.2
IBM1DeletionSrcGivenTgt:../ibm1.${src_lang}_given_en.gz#0.2
nbestWordPostLev:1#<ffval-wts>#<pfx>
nbestWordPostTrg:1#<ffval-wts>#<pfx>
nbestNgramPost:3#1#<ffval-wts>#<pfx>
nbestSentLenPost:1#<ffval-wts>#<pfx>
nbestWordPostSrc:1#<ffval-wts>#<pfx>
nbestPhrasePostSrc:1#<ffval-wts>#<pfx>
nbestPhrasePostTrg:1#<ffval-wts>#<pfx>
END

# Use configtool to get the list of basic decoder features
my $basic_features = `cd ${workdir}; configtool rescore-model:ffvals.gz canoe.ini`;

open( MODEL, "> ${workdir}/rescoring_model" );
print MODEL $basic_features;
print MODEL $model1;
close( MODEL );

system( "cd ${workdir}; bash $script" );

};


