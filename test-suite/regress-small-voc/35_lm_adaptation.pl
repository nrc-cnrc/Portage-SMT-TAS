#!/usr/bin/perl -w

# 03_gen_phr.pl - generate phrase based translation tables
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

my $workdir0 = "${base_work}/wk_${src_lang}";
my $workdir = "${workdir0}/lm_adaptation";
mkdir $workdir;


my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

#train some LMs
mkdir LMs

split -a 1 -dl 18000 ${corp}/europarl.de-en.en.lowercase LMs/train.data.en.
split -a 1 -dl 18000 ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase LMs/train.data.${src_lang}.

pushd LMs >& /dev/null
for n in 1 2 3;
do
   ngram-count -interpolate -kndiscount -order 3 -text train.data.en.\$n -lm \$n.lm.en
   ngram-count -interpolate -kndiscount -order 3 -text train.data.${src_lang}.\$n -lm \$n.lm.${src_lang}
done
popd >& /dev/null

# Get the LM's base name
ls -1 LMs/?.lm.en | cut -d'/' -f2- | perl -pe 's/.en\$//go;' > components

# for each sets
for n in test2000.${src_lang}.lowercase;
do
   f=${corp}/\$n
   weight_file=\$n.wts

   test -e \$weight_file && continue

   # Calculates the distance between source text and source lm and generates weights
   mx-calc-distances.sh \\
     -v \\
     -d LMs/ \\
     -e .${src_lang} \\
     em \\
     components \\
     \$f \\
   | mx-dist2weights -v normalize - > \$weight_file 

   # Links weights with the target lm
   mx-mix-models.sh \\
     -v \\
     -d LMs/ \\
     -e .en \\
     mixlm \\
     \$weight_file \\
     components \\
     \$f \\
     > \${weight_file}.mixlm
done

END

my $script = "${workdir}/${src_lang}_35_lm_adaptation";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

my $rc = system( "cd ${workdir}; bash $script" );

};
