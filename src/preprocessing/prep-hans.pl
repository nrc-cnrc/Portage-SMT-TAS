#!/usr/bin/perl -s

# @file prep-hans.pl 
# @brief Preprocess Hansard files in old (non-HTML) format.
# 
# @author George Foster
# 
# COMMENTS: 
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

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
printCopyright("prep-hans.pl", 2005);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


$HELP = "
prep-hans.pl [in [out]]

Preprocess a Hansard file.

Options:

-n Omit re-coded markup (source language, date, time, sect) in output.

";

if ($help || $h) {
    print $HELP;
    exit 0;
}
 
$in = shift || "-";
$out = shift || "-";
 
if (!open(IN, "<$in")) {die "Can't open $in for writing";}
if (!open(OUT, ">$out")) {die "Can't open $out for reading";}

# regexp for .BLABLA things to filter out. Need a list, because sometimes these
# codes are stuck to the following word. Don't forget to double escape (\\)
# special chars, because the first escape is removed in the assignment to
# $dotcodes.

$dotcodes = 
    "NONP[0-9]*|C\\.L\\.|BREV|10XBF|TPC|TOC[.]?|TCC|S\\.C\\.|CAPS|[0-9]+" .
    "|TUC|TCC|YEAS[0-9]*|NAYS[0-9]*|PAIR(ED|�S)[0-9]*|HEAD[RC]|SIGC" .
    "|\\[.*\\]|\\*space[0-9]*|[ ]";

# print "$dotcodes\n";

while (<IN>) {

    # don't move this block, or it'll affect the output markup
    s/L[<>]E/\n/go;
    s/[*]18(m<s)?/ /go;
    s/^<+//o;
    s/(.)[<>]/$1 /go;

    if (!$n) {
	s/^[.]\[(Translation|Fran�ais)\]/<srclang fr>/o;
	s/^[.]\[(Traduction|English)\]/<srclang en>/o;
	s/^[.]([0-9]+)\s*$/<time $1>/o;
	s/^[*]Date (.*)$/<date $1>/o;
	s/^\*\s+\*\s+\*/<sect>/o;
    }

#    s/\|([^|]+)\|/$1/go;	# |xxx| -> xxx
#    s/\\([^\\]+)\\/$1/go;	# \xxx\ -> xxx

    
    s/^[.]($dotcodes)\s*//o;    # remove .xxx  [at line start]
    s/^>[.]TCC//o;		# 1 special case
    s/^\s*>//o;			# remove >xxx  [at line start]
    s/^[*].*$//o;		# remove *xxx  [line start]

    s/[*]su(\w*)[*]xx/$1/go;
    s/[*](it|ro)//go;

    s/[\\|\[\]]//go;		# single | \ [ ] chars removed

    print OUT;
}

close IN;
close OUT;
