#!/usr/bin/perl -w

# 23_ranslate.pl - rescore translate on the test set
#                  29_rat_test.pl makes this script obsolete
#
# PROGRAMMER: Howard Johnson / Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, 2006, Conseil national de recherches du Canada / National Research Council of Canada

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

my $wfr = "${work}/wk_${fr}";
my $wfr_t = "../rtrain"; # Relative to $wfr_u
my $wfr_u = "${work}/wk_${fr}/rtest";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

rescore_translate        \\
  ${wfr_t}/rescoring_model        \\
  ${corp}/realtest2000.${fr}.lowercase text_en.nbest > test_en.out
bleumain test_en.out ${corp}/realtest2000.en.lowercase 
END

open( J1, "> ${wfr_u}/${fr}_23_translate" );
print J1 $job1;
close( J1 );

system( "cd ${wfr_u}; bash ${fr}_23_translate" );

};
