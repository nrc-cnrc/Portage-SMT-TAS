#!/usr/bin/perl -w

# 61_canoe_cube.pl - simple canoe run with default weights, using cube pruning
#
# PROGRAMMER: Howard Johnson / Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
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

my $w_dist;
my $w_wp;
my $w_lm;
my $w_tm;
my $w_seg;

foreach my $fr ( @fr ) {

open( PARM, "${work}/parm_${fr}.txt" );
while ( <PARM> ) {
  chomp;
  eval;
}
close( PARM );

my $wfr = "${work}/wk_${fr}";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

Q=0.5
if [ -r corpus.${fr}-en.bernoulli ]; then Q=`cat corpus.${fr}-en.bernoulli`; fi

seg_model="-segmentation-model bernoulli#0.2"
# seg_model="-segmentation-model bernoulli#\$Q"
# seg_model="-segmentation-model context#segmentation.left-right.freq"
# seg_model="-segmentation-model count"

canoe               \\
  -f canoe-cube.ini \\
  -verbose 1        \\
  -weight-d $w_dist \\
  -weight-w $w_wp   \\
  -weight-l $w_lm   \\
  -weight-t $w_tm   \\
  -weight-s $w_seg   \\
  \$seg_model \\
  < ${corp}/test2000.${fr}.lowercase 2> log.canoe-cube > test.out-cube
bleumain test.out-cube ${corp}/test2000.en.lowercase 
END

open( J1, "> ${wfr}/${fr}_61_canoe_cube" );
print J1 $job1;
close( J1 );

my $ini1 = << "END";
[ttable-limit] 30
[distortion-limit] 7
[ttable-file-s2t] phrases-GT.${fr}_given_en.gz
[ttable-file-t2s] phrases-GT.en_given_${fr}.gz
[lmodel-file] ${corp0}/europarl.en.srilm.binlm
[cube-pruning]
END

open( J1, "> ${wfr}/canoe-cube.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${wfr}; sh ${fr}_61_canoe_cube" );

};
