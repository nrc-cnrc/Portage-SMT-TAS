#!/usr/bin/perl -w

# 21_sanity.pl - "test" on dev, just to make sure things went OK
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
my $workdir  = "${workdir0}/rtrain";
my $rescore_training = "${workdir0}/rat_train";
mkdir $workdir;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

echo -n "Simple rescoring of the dev set "
rescore_translate        \\
  -p /${rescore_training}/workdir-test2000.${src_lang}.lowercase-200best/ \\
  ${rescore_training}/workdir-test2000.${src_lang}.lowercase-200best/rescoring_model.rat.out        \\
  ${corp}/test2000.${src_lang}.lowercase \\
  ${rescore_training}/workdir-test2000.${src_lang}.lowercase-200best/200best.gz \\
  > test_en.out \\
  2> log.rescore_translate \\
&& echo "OK" || echo "FAILED"

bleumain test_en.out ${corp}/test2000.en.lowercase

END

my $script = "${workdir}/${src_lang}_21_sanity";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

system( "cd ${workdir}; bash $script" );

};
