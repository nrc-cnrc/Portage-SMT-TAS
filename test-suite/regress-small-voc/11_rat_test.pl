#!/usr/bin/perl -w

# 29_rat_test.pl - test a rescoring model using rat.sh, which combines
#                  generating the n-best list, all the features, and doing the
#                  actual translation and evaluation
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
my $workdir0 = "${base_work}/wk_${src_lang}";
my $workdir = "${workdir0}/rat_test";
my $decoder_training = "${workdir0}/cow";
my $rescore_training = "../rat_train"; # relative to $workdir

mkdir $workdir;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

echo -n "Translating test set "
rat.sh                                  \\
    -lb                                 \\
    -n 3                                \\
    trans                               \\
    -f canoe.ini                        \\
    -K 200 -n 3                         \\
    $rescore_training/rescoring_model.out      \\
    ${corp}/realtest2000.${src_lang}.lowercase \\
    ${corp}/realtest2000.en.lowercase   \\
    &> log.rat_test                     \\
&& echo "OK" || echo "FAILED"
END

my $script = "${workdir}/${src_lang}_11_rat_test";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

if ( -r "${decoder_training}/canoe.ini.cow" ) {
    print "Using canoe.ini.cow found in ${decoder_training}\n";
    system( "cp ${decoder_training}/canoe.ini.cow ${workdir}/canoe.ini" );
} else {
    print "No canoe.ini.cow found in ${decoder_training}.\n";
    print "Run 05_cow.pl before this script!\n";
    exit 1;
}

system( "cd ${workdir}; bash $script" );

};


