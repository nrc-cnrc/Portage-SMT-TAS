#!/usr/bin/perl -s

# @file prep-and-tok-europarl.pl 
# @brief Preprocess Europarl (en/fr) files.
# 
# @author George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use locale;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils";
   }
}
use portage_utils;
printCopyright("prep-and-tok-europarl.pl", 2005);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

$HELP = "
prep-and-tok-europarl.pl [in [out]]

Pre-process and tokenize a Europarl corpus file: strip markup, fix
tokenization, and map to LC (the corpus is already nominally tokenized and
aligned).

";

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
$in = shift || "-";
$out = shift || "-";
 
if (!open(IN, "<$in")) {die "Error: Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Error: Can't open $out for reading";}

while (<IN>) {

    if (!/<.*>/) {

	$lower = lc;

	# l ' an -> l' an
	$lower =~ s/ ([cdjlmnst]|\S*qu|\S*-[dlm]) \' / \1\' /go;
	$lower =~ s/^([cdjlmnst]|\S*qu|\S*-[dlm]) \' /\1\' /go;
	$lower =~ s/aujourd \' hui/aujourd\'hui/go;

	# a-t-il -> a il (only a good idea if translation from French)
	#$lower =~ s/(\S+)-t-(\S+)/\1 \2/go;

	$lower =~ s/(\S+)-(je|tu|on|ils?|elles?|nous|vous|moi|lui|leur|les?|y)/\1 \2/go;

	print OUT $lower;
    } else {
	print OUT "\n";
    }
}

close IN;
close OUT;
