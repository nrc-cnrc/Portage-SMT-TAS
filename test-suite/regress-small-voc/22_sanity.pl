#!/usr/bin/perl -w

# 22_sanity.pl - "test" on dev, just to make sure things went OK
#
# PROGRAMMER: Howard Johnson / Eric Joanis
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
my $wfr_t = "${work}/wk_${fr}/rtrain";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

rescore_translate        \\
  rescoring_model        \\
  ${corp}/test2000.${fr}.lowercase text_en.nbest > test_en.out
bleumain test_en.out ${corp}/test2000.en.lowercase 
END

open( J1, "> ${wfr_t}/${fr}_22_sanity" );
print J1 $job1;
close( J1 );

system( "cd ${wfr_t}; bash ${fr}_22_sanity" );

};
