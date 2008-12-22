#!/usr/bin/perl -w

# 02_train_ibm.pl - train IBM translation tables
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

print "CORP $corp\nWORK $base_work\n";

my @src_lang = (
  'fr',
#  'de',
#  'es',
#  'fi',
);
if ( @ARGV ) { @src_lang = @ARGV }

foreach my $src_lang ( @src_lang ) {

my $workdir = "${base_work}/wk_${src_lang}";
print "W_LANG $workdir\n";

mkdir "${workdir}";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

train_ibm -v  -mimic he-lex              \\
  hmm.en_given_${src_lang}.gz                    \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.train_hmm.en_given_${src_lang}
END

my $script_foward = "${workdir}/${src_lang}_41_gen_hmm_f";
open( J1, "> $script_foward" );
print J1 $job1;
close( J1 );

print "Generating forward HMM tables for $src_lang...\n";
my $rc = system( "cd ${workdir}; bash $script_foward" );
print "RC = $rc\n";

my $job2 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

train_ibm -vr -mimic he-lex              \\
  hmm.${src_lang}_given_en.gz                    \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.train_hmm.${src_lang}_given_en
END

my $script_bakward = "${workdir}/${src_lang}_41_gen_hmm_r";
open( J2, "> $script_bakward" );
print J2 $job2;
close( J2 );

print "Generating backward HMM tables for $src_lang...\n";
$rc = system( "cd ${workdir}; bash $script_bakward" );
print "RC = $rc\n";

my $job3 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

echo -n "Creating a HMM GTSmoother and KNSmoother multiprob both direction phrase table "
gen_phrase_tables -v -w1 -m8 -1 ${src_lang} -2 en -ibm 2 -z \\
  -s GTSmoother -s KNSmoother   \\
  -multipr both                 \\
  -hmm  \\
  -o hmm.phrases-GT-KN              \\
  hmm.en_given_${src_lang}.gz        \\
  hmm.${src_lang}_given_en.gz        \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.gen_multi_prob_phrase_tables_hmm   \\
&& echo "OK" || echo "FAILED"

END

my $script = "${workdir}/${src_lang}_41_gen_hmm_phr";
open( J3, "> $script" );
print J3 $job3;
close( J3 );

print "Running gen_phrase_tables HMM for $src_lang...\n";
my $rc = system( "cd ${workdir}; bash $script" );
print "RC = $rc\n";

};
