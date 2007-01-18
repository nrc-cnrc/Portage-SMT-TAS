#!/usr/bin/perl -w

# 10_cow.pl - run COW - optimize weights for canoe
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

mkdir $wfr_r;
mkdir "$wfr_r/workdir";

# Will be run in directory $wfr_r
my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

cow.sh -v -filt -floor 2               \\
  -mad 1                               \\
  -parallel:"-n 4"                     \\
  -workdir workdir                     \\
  -nbest-list-size 100                 \\
  ${corp}/test2000.${fr}.lowercase     \\
  ${corp}/test2000.en.lowercase        \\
  ${corp}/test2000.en.lowercase        \\
  ${corp}/test2000.en.lowercase        \\
  &> log.cow
END

open( J1, "> ${wfr_r}/${fr}_10_rescore" );
print J1 $job1;
close( J1 );

my $Q=0.5;
if (open(B,"<${wfr}/corpus.${fr}-en.bernoulli")) {
    $Q = <B>;
    close B;
}; 
#    my ($seg_model, $seg_arg) = ("bernoulli", $Q);
#    my ($seg_model, $seg_arg) = ("bernoulli", 0.2);
    my ($seg_model, $seg_arg) = ("none", "whatever");
#    my ($seg_model, $seg_arg) = ("count", "whatever");
#    my ($seg_model, $seg_arg) = ("context", "../segmentation.left-right.freq");
# my $dist_model = "WordDisp_Prob:../dst_temp/corpus.${fr}-en.distortion_a";

my $dist_model = "WordDisplacement";
# my $dist_model = "none";

my $ini1 = << "END";
[ttable-limit] 30
[distortion-limit] 7
[ttable-multi-prob] ../phrases-GT-KN.${fr}2en.gz
[lmodel-file] ${corp0}/europarl.en.srilm
[segmentation-model] ${seg_model}
[segmentation-args] ${seg_arg}
[distortion-model] ${dist_model}
END

open( J1, "> ${wfr_r}/canoe.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${wfr_r}; bash ${fr}_10_rescore" );

};
