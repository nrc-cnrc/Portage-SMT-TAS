#!/usr/bin/perl -w

# 30_summary.pl - display a summary of evaluation results of the test suite
#
# PROGRAMMER: Eric Joanis
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
my $wfr_t = "rat_train"; # relative to $wfr
my $wfr_u = "rat_test";  # relative to $wfr

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

cd $wfr_t
echo
echo Sanity check - evaluation of 1-best output from canoe on dev:
bleumain test2000.${fr}.lowercase.1best    \\
    ${corp}/test2000.en.lowercase

echo
echo Sanity check - evaluation of rescoring output on dev:
rescore_translate -p test2000.${fr}.lowercase. rescoring_model.w     \\
    ${corp}/test2000.${fr}.lowercase    \\
    test2000.${fr}.lowercase.nbest.gz      \\
    2> /dev/null                        \\
    | bleumain - ${corp}/test2000.en.lowercase

cd ../$wfr_u
echo
echo Test results - 1-best output from canoe on test:
bleumain realtest2000.${fr}.lowercase.1best \\
    ${corp}/realtest2000.en.lowercase

echo
echo Test results - rescoring output on test:
bleumain realtest2000.${fr}.lowercase.rat  \\
    ${corp}/realtest2000.en.lowercase
END

open( J1, "> ${wfr}/${fr}_30_summary" );
print J1 $job1;
close( J1 );

system( "cd ${wfr}; bash ${fr}_30_summary" );

};


