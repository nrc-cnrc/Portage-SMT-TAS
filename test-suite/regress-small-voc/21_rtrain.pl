#!/usr/bin/perl -w

# 21_rtrain.pl - train a rescoring model
#                28_rat_train.pl makes this script obsolete
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
my $wfr_t = "${work}/wk_${fr}/rtrain";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

rescore_train -vn        \\
  rescoring_model.ini    \\
  rescoring_model        \\
  ${corp}/test2000.${fr}.lowercase  \\
  text_en.nbest                  \\
  ${corp}/test2000.en.lowercase     \\
  ${corp}/test2000.en.lowercase     \\
  ${corp}/test2000.en.lowercase     \\
  &> log.rescore_train
END

open( J1, "> ${wfr_t}/${fr}_21_rtrain" );
print J1 $job1;
close( J1 );

my $ini1 = << "END";
FileFF:ff.distortion 1
FileFF:ff.wordpenalty 1
FileFF:ff.segmentation 1
FileFF:ff.lm 1
FileFF:ff.phrase-tm 1
FileFF:ff.LengthFF 1
FileFF:ff.NgramFF 1
FileFF:ff.IBM1TgtGivenSrc 1
FileFF:ff.IBM1SrcGivenTgt 1
FileFF:ff.IBM2TgtGivenSrc 1
FileFF:ff.IBM2SrcGivenTgt 1
FileFF:ff.WTransIBM1TgtGivenSrc 1
FileFF:ff.WTransIBM1SrcGivenTgt 1
END

open( J1, "> ${wfr_t}/rescoring_model.ini" );
print J1 $ini1;
close( J1 );

system( "cd ${wfr_t}; bash ${fr}_21_rtrain" );

};
