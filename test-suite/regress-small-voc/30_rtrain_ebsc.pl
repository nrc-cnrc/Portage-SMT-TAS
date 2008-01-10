#!/usr/bin/perl -w

# 21_rtrain.pl - train a rescoring model with expectation-based stopping criterion
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
my $workdir  =  "${workdir0}/rtrain_ebsc";
my $trans_dir = "${workdir0}/rat_train/workdir-test2000.${src_lang}.lowercase-200best";
mkdir $workdir; 

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

ln -sf ../rat_train/workdir-test2000.${src_lang}.lowercase-200best workdir

echo -n "Training a rescoring-model with expectation based stopping criterion "
rescore_train -vn        \\
  -e -r 5                \\
  -p workdir/            \\
  rescoring_model.ini    \\
  rescoring_model.ebsc   \\
  ${corp}/test2000.${src_lang}.lowercase  \\
  workdir/200best                \\
  ${corp}/test2000.en.lowercase  \\
  ${corp}/test2000.en.lowercase  \\
  ${corp}/test2000.en.lowercase  \\
  &> log.rescore_train           \\
&& echo "OK" || echo "FAILED"

END

my $script = "${workdir}/${src_lang}_30_rtrain_ebsc";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

if ( -r "${trans_dir}/rescoring_model.rat.out" ) {
    print "Using coring_model.rat.out found in ${trans_dir}\n";
    #system( "cp ${trans_dir}/rescoring_model.rat.out ${workdir}/rescoring_model.ini" );
    system( "cut -f1 -d' ' ${trans_dir}/rescoring_model.rat.out > ${workdir}/rescoring_model.ini" );
} else {
    print "No rescoring_model.rat.out found in ${trans_dir}.\n";
    print "Run 10_rat_train.pl before this script!\n";
    exit 1;
}

system( "cd ${workdir}; bash $script" );

};
