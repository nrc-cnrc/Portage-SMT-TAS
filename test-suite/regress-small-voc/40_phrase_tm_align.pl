#!/usr/bin/perl -w

# 40_phrase_tm_align.pl - Finds phrase alignments.
# Use the phrase tables to find phrase alignments for given source and target files.
#
# PROGRAMMER: Samuel Larkin
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

my $workdir0  = "${base_work}/wk_${src_lang}";
my $workdir   = "${workdir0}/phrase_tm_align";
my $ref_dir   = "${workdir0}/cow";
my $trans_dir = "${workdir0}/rat_test";
mkdir $workdir;

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

echo -n "finding phrase alignments with phrase_tm_align for the test set "
phrase_tm_align \\
  -f canoe.ini \\
  -ref ${corp}/realtest2000.en.lowercase \\
  -out realtest2000.align \\
  < ${trans_dir}/realtest2000.${src_lang}.lowercase.rat \\
  >& log.phrase_tm_align \\
&& echo "OK" || echo "FAILED"

END

my $script = "${workdir}/${src_lang}_40_phrase_tm_align";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

if ( -r "${ref_dir}/canoe.ini.cow" ) {
    print "Using canoe.ini.cow found in ${ref_dir}\n";
    system( "cp ${ref_dir}/canoe.ini.cow ${workdir}/canoe.ini" );
} else {
    print "No canoe.ini.cow found in ${ref_dir}.\n";
    print "Run 05_cow.pl before this script!\n";
    exit 1;
}

if ( -r "${trans_dir}/realtest2000.${src_lang}.lowercase.rat" ) {
    print "Using test_en.out found in ${trans_dir}\n";
    system( "cp ${trans_dir}/realtest2000.${src_lang}.lowercase.rat ${workdir}/realtest2000.${src_lang}.lowercase.rat" );
} else {
    print "No test_en.out found in ${trans_dir}.\n";
    print "Run 10_rat_test.pl before this script!\n";
    exit 1;
}

system( "cd ${workdir} && bash $script" );

};
