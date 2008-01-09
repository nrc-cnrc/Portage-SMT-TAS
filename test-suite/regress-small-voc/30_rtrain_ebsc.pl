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
mkdir $workdir; 

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

ln -sf ../rat_train/workdir-test2000.${src_lang}.lowercase-200best workdir

echo -n "Training a rescoring-model with expectation based stopping criterion "
rescore_train -vn        \\
  -e -r 50               \\
  -p workdir/            \\
  rescoring_model.ini    \\
  rescoring_model        \\
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

my $ini1 = << "END";
FileFF:ffvals,1
FileFF:ffvals,2
FileFF:ffvals,3
FileFF:ffvals,4
FileFF:ffvals,5
FileFF:ff.LengthFF.gz
FileFF:ff.ParMismatch.gz
END

open( J1, "> ${workdir}/rescoring_model.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${workdir}; bash $script" );

};
