#!/usr/bin/perl -w

# 31_rtrain_rnd_w.pl - train a rescoring model with user specific random
#                      distribution.  Also testing the support of , in a
#                      feature file name, and that comments can be inserted in
#                      a rescoring_model file.
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
my $workdir  = "${workdir0}/rtrain_rnd_w";
mkdir $workdir;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

ln -sf ../rat_train/workdir-test2000.${src_lang}.lowercase-200best workdir
ln -sf ff.ParMismatch.gz workdir/ff.Par,Mismatch.gz

echo -n "Using rescore_train with users' random distributions "
rescore_train -vn        \\
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


my $script = "${workdir}/${src_lang}_31_rtrain_rnd_w";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

my $ini1 = << "END";
FileFF:ffvals,1 U(-1,4)
FileFF:ffvals,2 N(4, 1)
FileFF:ffvals,3 U(0,5)
FileFF:ffvals,4 N(2.3,.8)
FileFF:ffvals,5 1
FileFF:ff.LengthFF.gz 1
FileFF:ff.ParMismatch.gz 1
# testing that , is allowed in file name
FileFF:ff.Par,Mismatch.gz 1
END

open( J1, "> ${workdir}/rescoring_model.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${workdir}; bash $script" );

};
