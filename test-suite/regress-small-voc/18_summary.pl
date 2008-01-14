#!/usr/bin/perl -w

# 18_summary.pl - display a summary of evaluation results of the test suite
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
my $workdir   = "${base_work}/wk_${src_lang}";
my $wfr_t = "rat_train"; # relative to $workdir
my $wfr_u = "rat_test";  # relative to $workdir

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

cd $wfr_t
echo
echo Sanity check - evaluation of 1-best output from canoe on dev:
bleumain                                                  \\
  workdir-test2000.${src_lang}.lowercase-200best/1best    \\
  ${corp}/test2000.en.lowercase

echo
echo Sanity check - evaluation of rescoring output on dev:
rescore_translate                                                        \\
  -p workdir-test2000.${src_lang}.lowercase-200best/                     \\
  workdir-test2000.${src_lang}.lowercase-200best/rescoring_model.rat.out \\
  ${corp}/test2000.${src_lang}.lowercase                                 \\
  workdir-test2000.${src_lang}.lowercase-200best/*best.gz                \\
  2> log.rescore_translate                                               \\
  | bleumain - ${corp}/test2000.en.lowercase

cd ../$wfr_u
echo
echo Test results - 1-best output from canoe on test:
bleumain                                                   \\
  workdir-realtest2000.${src_lang}.lowercase-200best/1best \\
  ${corp}/realtest2000.en.lowercase

echo
echo Test results - rescoring output on test:
bleumain                                  \\
  realtest2000.${src_lang}.lowercase.rat  \\
  ${corp}/realtest2000.en.lowercase
END

my $script = "${workdir}/${src_lang}_18_summary";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

system( "cd ${workdir}; bash $script" );

};


