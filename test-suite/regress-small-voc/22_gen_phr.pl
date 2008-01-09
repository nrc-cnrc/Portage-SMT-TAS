#!/usr/bin/perl -w

# 41_gen_phr.pl - generate phrase based translation tables
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

my $workdir = "${base_work}/wk_${src_lang}";


my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

# This script generates many redundant phrase tables, for testing purposes.

echo -n "Creating a GTSmoother tmtext phrase table "
gen_phrase_tables -v -w1 -m8 -1 ${src_lang} -2 en -ibm 2 -z \\
  -s "GTSmoother"               \\
  -tmtext                       \\
  -o phrases-GT                 \\
  ibm2.en_given_${src_lang}.gz        \\
  ibm2.${src_lang}_given_en.gz        \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.gen_gt_phrase_tables   \\
&& echo "OK" || echo "FAILED"

echo -n "Creating a GTSmoother multipr phrase table "
gen_phrase_tables -v -w1 -m8 -1 ${src_lang} -2 en -ibm 2 -z \\
  -s "GTSmoother"               \\
  -multipr both                 \\
  -o phrases-GT                 \\
  ibm2.en_given_${src_lang}.gz        \\
  ibm2.${src_lang}_given_en.gz        \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.gen_gt_phrase_tables   \\
&& echo "OK" || echo "FAILED"

echo -n "Creating a KNSmoother tmtext phrase table "
gen_phrase_tables -v -w1 -m8 -1 ${src_lang} -2 en -ibm 2 -z \\
  -s "KNSmoother"               \\
  -tmtext                       \\
  -o phrases-KN                 \\
  ibm2.en_given_${src_lang}.gz        \\
  ibm2.${src_lang}_given_en.gz        \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.gen_kn_phrase_tables   \\
&& echo "OK" || echo "FAILED"

echo -n "Creating a KNSmoother multipr phrase table "
gen_phrase_tables -v -w1 -m8 -1 ${src_lang} -2 en -ibm 2 -z \\
  -s "KNSmoother"               \\
  -multipr both                 \\
  -o phrases-KN                 \\
  ibm2.en_given_${src_lang}.gz        \\
  ibm2.${src_lang}_given_en.gz        \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.gen_kn_phrase_tables   \\
&& echo "OK" || echo "FAILED"

echo -n "Creating a LeaveOneOut tmtext phrase table "
gen_phrase_tables -v -w1 -m8 -1 ${src_lang} -2 en -ibm 2 -z \\
  -s "LeaveOneOut 1"            \\
  -tmtext                       \\
  -o phrases-l1o                \\
  ibm2.en_given_${src_lang}.gz        \\
  ibm2.${src_lang}_given_en.gz        \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.gen_l1o_tables         \\
&& echo "OK" || echo "FAILED"

echo -n "Joining to phrase tables into one "
join_phrasetables phrases-GT.en2fr.gz phrases-KN.en2fr.gz \\
   2> /dev/null \\
   | gzip > phrases-GT-KN.en2fr.joined.gz \\
&& echo "OK" || echo "FAILED"

echo -n "Checking the joined phrase table against a generated one "
diff-round.pl 'LC_ALL=C gzip -cqfd phrases-GT-KN.en2fr.gz | sort |' 'LC_ALL=C gzip -cqfd phrases-GT-KN.en2fr.joined.gz | sort |' \\
2> /dev/null \\
&& echo "OK" || echo "FAILED"

END

my $script = "${workdir}/${src_lang}_22_gphr";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

print "Running gen_phrase_tables for $src_lang...\n";
my $rc = system( "cd ${workdir}; bash $script" );
print "RC = $rc\n";

};
