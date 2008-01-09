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

train_ibm -v  -s                            \\
  ibm1.en_given_${src_lang}.gz                    \\
  ibm2.en_given_${src_lang}.gz                    \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.train_ibm.en_given_${src_lang}
END

my $script_foward = "${workdir}/${src_lang}_02_tribm_f";
open( J1, "> $script_foward" );
print J1 $job1;
close( J1 );

print "Generating forward IBM tables for $src_lang...\n";
my $rc = system( "cd ${workdir}; bash $script_foward" );
print "RC = $rc\n";

my $job2 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

train_ibm -vr -s                            \\
  ibm1.${src_lang}_given_en.gz                    \\
  ibm2.${src_lang}_given_en.gz                    \\
  ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase \\
  ${corp}/europarl.${src_lang}-en.en.lowercase    \\
  &> log.train_ibm.${src_lang}_given_en
END

my $script_bakward = "${workdir}/${src_lang}_02_tribm_r";
open( J2, "> $script_bakward" );
print J2 $job2;
close( J2 );

print "Generating backward IBM tables for $src_lang...\n";
$rc = system( "cd ${workdir}; bash $script_bakward" );
print "RC = $rc\n";

};
