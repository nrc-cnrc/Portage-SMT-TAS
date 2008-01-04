#!/usr/bin/perl -w

# 08_rcanoe.pl - generate nbest lists and features for rescore translation
#                29_rat_test.pl makes this script obsolete
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
my $wfr_r = "${wfr}/cow";
my $wfr_u = "${wfr}/filter_models";

mkdir $wfr_u;

# Will be run in directory $wfr_u
my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

FILT_CMD="filter_models -c -f canoe.ini -suffix .NEW"

run() {
   NAME=\$1
   CMD_OPTS=\$2
   echo "Filtering \$NAME"
   (time \
      \$FILT_CMD \$CMD_OPTS \\
      && mv tmp.NEW \$NAME \\
      && wc \$NAME) &> log.\$NAME \\
   || echo "Failed filtering \$NAME"
}

# Reducing source sentence vocabulary
head ${corp}/test2000.${fr}.lowercase > src

# LM filtering
echo "Filtering LM"
(time \$FILT_CMD -L < src \\
   && mv europarl.en.srilm.NEW LM.filtered) >& log.filter_models.lm \\
|| echo "Failed filtering LM"

echo "Filtering LM with phaseIIb"
(time \$FILT_CMD -L -phaseIIb < src \\
   && mv europarl.en.srilm.NEW LM.filtered.phaseIIb) >& log.filter_models.lm.phaseIIb \\
|| echo "Failed filtering LM phaseIIb"


# TM filtering
run online.hard.complete "-no-src-grep -tm-online -hard-limit tmp"
run online.hard.incomplete "-input src -tm-online -hard-limit tmp"
run online.soft.complete "-no-src-grep -tm-online -soft-limit tmp"
run online.soft.incomplete "-input src -tm-online -soft-limit tmp"

run notonline.hard.complete "-no-src-grep -hard-limit tmp"
run notonline.hard.incomplete "-input src -hard-limit tmp"
run notonline.soft.complete "-no-src-grep -soft-limit tmp"
run notonline.soft.incomplete "-input src -soft-limit tmp"


RC=0
echo "Comparing notonline.hard.complete online.hard.complete"
diff-phrasetable.pl notonline.hard.complete   online.hard.complete || RC=1

echo "Comparing notonline.hard.incomplete online.hard.incomplete"
diff-phrasetable.pl notonline.hard.incomplete online.hard.incomplete || RC=2

echo "notonline.soft.complete online.soft.complete"
diff-phrasetable.pl notonline.soft.complete   online.soft.complete || RC=3

echo "notonline.soft.incomplete online.soft.incomplete"
diff-phrasetable.pl notonline.soft.incomplete online.soft.incomplete || RC=4

exit \$RC
END

open( J1, "> ${wfr_u}/${fr}_70_filter_models" );
print J1 $job1;
close( J1 );


my $canoe_ini = << "END";
[ttable-limit] 30
[distortion-limit] 7
[ttable-multi-prob] ../phrases-GT-KN.fr2en.gz
[lmodel-file] ${corp0}/europarl.en.srilm
[weight-t] 0.4:1
END

open( J1, "> ${wfr_u}/canoe.ini" );
print J1 $canoe_ini;
close( J1 );

system( "cd ${wfr_u}; bash ${fr}_70_filter_models" );

};
