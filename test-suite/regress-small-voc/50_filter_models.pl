#!/usr/bin/perl -w

# 70_filter_models - Exercises the phrase tables and language model filtering
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
my $workdir  = "${workdir0}/filter_models";

mkdir $workdir;

# Will be run in directory $workdir
my $job1 = << "END";
#!/bin/bash
LANG=en_US.ISO-8859-1

FILT_CMD="filter_models -c -f canoe.ini -suffix .NEW"

run() {
   NAME=\$1
   CMD_OPTS=\$2
   echo -n "Filtering \$NAME "
   (time \
      \$FILT_CMD \$CMD_OPTS \\
      && mv tmp.NEW \$NAME \\
      && wc \$NAME) &> log.\$NAME \\
   && echo "OK" || echo "FAILED"
}

diff() {
   echo -n "Comparing \$1 \$2 "
   diff-round.pl -prec 4 "LC_ALL=C gzip -cqfd \$1 | sort |" "LC_ALL=C gzip -cqfd \$2 | sort |" \\
   2> /dev/null \\
   | wc
}

# Reducing source sentence vocabulary
head ${corp}/test2000.${src_lang}.lowercase > src

# LM filtering
echo -n "Filtering LM "
(time \$FILT_CMD -L < src \\
   && mv europarl.en.srilm.NEW LM.filtered) >& log.filter_models.lm \\
&& echo "OK" || echo "FAILED"


# TM filtering
run online.hard.complete "-no-src-grep -tm-online -hard-limit tmp"
run online.hard.incomplete "-input src -tm-online -hard-limit tmp"
run online.soft.complete "-no-src-grep -tm-online -soft-limit tmp"
run online.soft.incomplete "-input src -tm-online -soft-limit tmp"

run notonline.hard.complete "-no-src-grep -hard-limit tmp"
run notonline.hard.incomplete "-input src -hard-limit tmp"
run notonline.soft.complete "-no-src-grep -soft-limit tmp"
run notonline.soft.incomplete "-input src -soft-limit tmp"


diff notonline.hard.complete   online.hard.complete
diff notonline.hard.incomplete online.hard.incomplete
diff notonline.soft.complete   online.soft.complete
diff notonline.soft.incomplete online.soft.incomplete

END

my $script = "${workdir}/${src_lang}_50_filter_models";
open( J1, "> $script" );
print J1 $job1;
close( J1 );


my $canoe_ini = << "END";
[ttable-limit] 30
[distortion-limit] 7
[ttable-multi-prob] ../phrases-GT-KN.${src_lang}2en.gz
[lmodel-file] ${corp0}/europarl.en.srilm
[weight-t] 0.4:1
END

open( J1, "> ${workdir}/canoe.ini" );
print J1 $canoe_ini;
close( J1 );

system( "cd ${workdir}; bash $script" );

};
