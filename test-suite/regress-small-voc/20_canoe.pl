#!/usr/bin/perl -w

# 20_canoe.pl - simple canoe run with default weights
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

my $w_dist;
my $w_wp;
my $w_lm;
my $w_tm;
my $w_seg;

foreach my $src_lang ( @src_lang ) {

open( PARM, "${base_work}/parm_${src_lang}.txt" );
while ( <PARM> ) {
  chomp;
  eval;
}
close( PARM );

my $workdir = "${base_work}/wk_${src_lang}";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

Q=0.5
if [ -r corpus.${src_lang}-en.bernoulli ]; then Q=`cat corpus.${src_lang}-en.bernoulli`; fi

seg_model="-segmentation-model bernoulli#0.2"
# seg_model="-segmentation-model bernoulli#\$Q"
# seg_model="-segmentation-model context#segmentation.left-right.freq"
# seg_model="-segmentation-model count"

echo -n "Translating dev set "
canoe               \\
  -f canoe.ini      \\
  -verbose 1        \\
  -weight-d $w_dist \\
  -weight-w $w_wp   \\
  -weight-l $w_lm   \\
  -weight-t $w_tm   \\
  -weight-s $w_seg  \\
  \$seg_model       \\
  -input ${corp}/test2000.${src_lang}.lowercase \\
  2> log.canoe > test.out \\
&& echo "OK" || echo "FAILED"

bleumain test.out ${corp}/test2000.en.lowercase 

END

my $script = "${workdir}/${src_lang}_20_canoe";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

my $ini1 = << "END";
[ttable-limit] 30
[distortion-limit] 7
[ttable-multi-prob] phrases-GT-KN.${src_lang}2en.gz
[lmodel-file] ${corp0}/europarl.en.srilm
END

open( J1, "> ${workdir}/canoe.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${workdir}; bash $script" );

};
