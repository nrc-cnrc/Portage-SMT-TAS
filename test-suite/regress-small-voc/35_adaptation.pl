#!/usr/bin/perl -w

# 35_adaptation.pl - Language model & phrase table adaptation
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

my $workdir0 = "${base_work}/wk_${src_lang}";
my $workdir = "${workdir0}/adaptation";
mkdir $workdir;

my $lm_type = "";
#my $lm_type = "-kndiscount";
#my $lm_type = "-interpolate -kndiscount";
#my $lm_type = "-interpolate -kndiscount1 2 -kndiscount2 2 -kndiscount3 2";

my $MODELS_DIR = "models";

my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

MODELS=${MODELS_DIR}
num_lines=19000

#train some models
test -d \$MODELS || mkdir \$MODELS

split -a 1 -dl \$num_lines ${corp}/europarl.de-en.en.lowercase  \$MODELS/train.data.en.
split -a 1 -dl \$num_lines ${corp}/europarl.${src_lang}-en.${src_lang}.lowercase  \$MODELS/train.data.${src_lang}.


# Making all adaptation models
pushd \$MODELS >& /dev/null

# Get the LM's base name
ls -1 train.data.en.? | perl -pe 's/^train.data.en.//go;' > components

for n in `cat components`;
do
   echo -n "Training en language model for chunk \$n "
   ngram-count $lm_type -order 3 -text train.data.en.\$n -lm \$n.lm.en &> log.\$n.lm.en \\
   && echo "OK" || echo "FAILED"

   echo -n "Training ${src_lang} language model for \$n "
   ngram-count $lm_type -order 3 -text train.data.${src_lang}.\$n -lm \$n.lm.${src_lang} &> log.\$n.lm.${src_lang} \\
   && echo "OK" || echo "FAILED"

   echo -n "Training phrase tables for chunk \$n "
   gen_phrase_tables \\
      -v -w1 -m8 -ibm 2 -z \\
      -1 ${src_lang} -2 en \\
      -s GTSmoother -s KNSmoother   \\
      -multipr fwd                  \\
      -o \$n.ibm2.phrases-GT-KN        \\
      ../../ibm2.en_given_${src_lang}.gz  \\
      ../../ibm2.${src_lang}_given_en.gz  \\
      train.data.en.\$n                \\
      train.data.${src_lang}.\$n       \\
      &> log.\$n.gen_multi_prob_phrase_tables \\
   && echo "OK" || echo "FAILED"
done

# for each sets
for n in test2000.${src_lang}.lowercase;
do
   f=${corp}/\$n
   weight_file=\$n.wts

   #test -e \$weight_file && continue

   echo -n "Calculating the distance between source text and source lm and generating weights "
   mx-calc-distances.sh \\
      -v \\
      -e .lm.${src_lang} \\
      em \\
      components \\
      \$f \\
   | mx-dist2weights -v normalize - > \$weight_file \\
   && echo "OK" || echo "FAILED"

   echo -n "Generating mix language model "
   mx-mix-models.sh \\
      -v \\
      -d `pwd`/ \\
      -e .lm.en \\
      mixlm \\
      \$weight_file \\
      components \\
      \$f \\
      2> log.mix.languagemodel \\
      > \${weight_file}.mixlm \\
   && echo "OK" || echo "FAILED"

   echo -n "Generating mix phrase table "
   mx-mix-models.sh \\
      -v \\
      -e .ibm2.phrases-GT-KN.${src_lang}2en.gz \\
      rf \\
      \$weight_file \\
      components \\
      \$f \\
      2> log.mix.phrasetables \\
      \| gzip \\
      \> \$n.ibm2.phrases-GT-KN.${src_lang}2en.gz \\
   && echo "OK" || echo "FAILED"
done
popd >& /dev/null
#exit # SAM DEBUG

echo -n "Training a simple decoder model with mix models "
test -d workdir || mkdir workdir
cow.sh -v -filt -floor 2               \\
  -I-really-mean-cow                   \\
  -lb                                  \\
  -mad 1                               \\
  -parallel:"-n 4"                     \\
  -workdir workdir                     \\
  -nbest-list-size 100                 \\
  ${corp}/test2000.${src_lang}.lowercase     \\
  ${corp}/test2000.en.lowercase        \\
  ${corp}/test2000.en.lowercase        \\
  ${corp}/test2000.en.lowercase        \\
  &> log.cow                           \\
&& echo "OK" || echo "FAILED"
END

my $script = "${workdir}/${src_lang}_35_adaptation";
open( J1, "> $script" );
print J1 $job1;
close( J1 );

my $ini1 = << "END";
[ttable-limit] 30
[ttable-multi-prob] ../phrases-GT-KN.${src_lang}2en.gz:${MODELS_DIR}/test2000.${src_lang}.lowercase.ibm2.phrases-GT-KN.${src_lang}2en.gz
[lmodel-file] ${corp0}/europarl.en.srilm:${MODELS_DIR}/test2000.${src_lang}.lowercase.wts.mixlm
[segmentation-model] none#whatever
[distortion-model] WordDisplacement
[distortion-limit] 7
END

open( J1, "> ${workdir}/canoe.ini" );
print J1 $ini1;
close( J1 );
my $rc = system( "cd ${workdir} && bash $script" );

};
