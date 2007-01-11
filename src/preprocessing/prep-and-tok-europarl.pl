#!/usr/bin/perl -s

# prep-europarl.pl Preprocess Europarl (en/fr) files
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use locale;

print STDERR "prep-europarl.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

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
 
if (!open(IN, "<$in")) {die "Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Can't open $out for reading";}

while (<IN>) {

    if (!/<.*>/) {

	$lower = lc;

	# l ' an -> l' an
	$lower =~ s/ ([cdjlmnst]|\S*qu|\S*-[dlm]) \' / \1\' /go;
	$lower =~ s/^([cdjlmnst]|\S*qu|\S*-[dlm]) \' /\1\' /go;
	$lower =~ s/aujourd \' hui/aujourd\'hui/go;

	# a-t-il -> a il
	$lower =~ s/(\S+)-t-(\S+)/\1 \2/go;

	$lower =~ s/(\S+)-(je|tu|on|ils?|elles?|nous|vous|moi|lui|leur|les?|y)/\1 \2/go;

	print OUT $lower;
    } else {
	print OUT "\n";
    }
}

close IN;
close OUT;
