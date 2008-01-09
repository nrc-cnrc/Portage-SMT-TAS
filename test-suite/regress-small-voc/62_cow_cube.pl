#!/usr/bin/perl -w

# 62_cow_cube.pl - run COW - optimize weights for canoe, using cube pruning
#
# PROGRAMMER: Howard Johnson / Eric Joanis / Samuel Larkin
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
my $workdir  = "${workdir0}/cow-cube";

mkdir $workdir;
mkdir "$workdir/workdir";

# Will be run in directory $workdir
my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

echo -n "Training decoder model with cube pruning "
cow.sh -v -filt -floor 2               \\
  -mad 1                               \\
  -parallel:"-n 2"                     \\
  -workdir workdir                     \\
  -nbest-list-size 100                 \\
  ${corp}/test2000.${src_lang}.lowercase     \\
  ${corp}/test2000.en.lowercase        \\
  ${corp}/test2000.en.lowercase        \\
  ${corp}/test2000.en.lowercase        \\
  >& log.cow                           \\
&& echo "OK" || echo "FAILED"

END

my $script = "${workdir}/${src_lang}_62_cow_cube";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

my $Q=0.5;
if (open(B,"<${workdir0}/corpus.${src_lang}-en.bernoulli")) {
    $Q = <B>;
    close B;
}; 
#    my $seg_model = "bernoulli#$Q";
#    my $seg_model = "bernoulli#0.2";
    my $seg_model = "none";
#    my $seg_model = "count";
#    my $seg_model = "context#../segmentation.left-right.freq";
# my $dist_model = "WordDisp_Prob#../dst_temp/corpus.${src_lang}-en.distortion_a";

my $dist_model = "WordDisplacement";
# my $dist_model = "none";

my $ini1 = << "END";
[ttable-limit] 30
[distortion-limit] 7
[ttable-multi-prob] ../phrases-GT-KN.${src_lang}2en.gz
[lmodel-file] ${corp0}/europarl.en.srilm
[segmentation-model] ${seg_model}
[distortion-model] ${dist_model}
[cube-pruning]
END

open( J1, "> ${workdir}/canoe.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${workdir}; bash $script" );

};
