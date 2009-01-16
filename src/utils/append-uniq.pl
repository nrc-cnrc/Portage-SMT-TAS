#!/usr/bin/perl
# $Id$

# @file append-uniq.pl 
# @brief Appends non duplicate lines from addnbest and addffvals files to nbest
# and ffvals, where a duplicate is a line which is identical to another in
# *both* files at the same time.
#
# @author Samuel Larkin
#
# COMMENTS:
#
# Samuel Larkin
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
   append-uniq.pl, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada

   Usage: $0 [-h(elp)] [-v(erbose)] -nbest=file -ffvals=file
          -addnbest=file -addffvals=file

   Appends non duplicate lines from addnbest and addffvals files to nbest and
   ffvals, where a duplicate is a line which is identical to another in *both*
   files at the same time.

   Warning: nbest and ffvals must be duplicate free, if not use use this
   program to remove duplicates as follow:
      append-uniq.pl -nbest=<emptyfile> -ffvals=<emptyfile>
         -addnbest=nbest -addffvals=ffvals

   Options:

   -h(elp):      print this help message
   -v(erbose):   increment the verbosity level by 1 (may be repeated)
   -d(ebug):     print debugging information

   ";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   "nbest=s"     => \my $nbest,
   "addnbest=s"  => \my $addnbest,
   "ffvals=s"    => \my $ffvals,
   "addffvals=s" => \my $addffvals,
   help          => sub { usage },
   verbose       => sub { ++$verbose },
   quiet         => sub { $verbose = 0 },
   debug         => \my $debug,
) or usage;

# This script is called once per sentence by cow.sh, don't pollute the log!
print STDERR "append-uniq.pl, NRC-CNRC, (c) 2004 - 2009, Her Majesty in Right of Canada\n"
   if ($verbose > 1);

if ( $debug ) {
   no warnings;
   print STDERR "
   nbest       = $nbest
   ffvals      = $ffvals
   addnbest    = $addnbest
   addffvals   = $addffvals
   verbose     = $verbose
   debug       = $debug

";
}


open(NBEST, "gzip -cqfd $nbest |") or die "Can't open $nbest for reading: $!\n";
open(FFVALS, "gzip -cqfd $ffvals |") or die "Can't open $ffvals for reading: $!\n";

# hash of hashes: $seen{$line1}{$line2} exists if $line1 exists in $file1 at
# the same position as $line2 in $file2.
my %seen;
my $iniLine = 0;
my $iniDup = 0;
my $n;
my $f;
while (defined($n = <NBEST>) and defined($f = <FFVALS>)) {
   chomp($n);
   chomp($f);
   if ( length($n) )
   {
      if ( ! exists($seen{$n}{$f}) ) {
         ++$iniLine;
         # This is not a duplicate line
         $seen{$n}{$f} = 1;
      } else {
         ++$iniDup;
         print(STDERR "WARNING: The following sentence was found to be a duplicate $f $n\n")
            if ($verbose > 1);
      }
   }
}
die "FATAL ERROR: $nbest is not of the same length as $ffvals\n"
   if (defined(<NBEST>) or defined(<FFVALS>));

close(NBEST);
close(FFVALS);



my $NBESTDF;
if ($nbest =~ /.gz/) {
   $nbest =~ s/(.*\.gz)\s*$/| gzip -cqf >> $1/;
   open($NBESTDF, $nbest)
      or die "Can't open output file $nbest\n";
}
else {
   open($NBESTDF, ">>$nbest")
      or die "Can't open output file $nbest\n";
}
my $FFVALSDF;
if ($ffvals =~ /.gz/) {
   $ffvals =~ s/(.*\.gz)\s*$/| gzip -cqf >> $1/;
   open($FFVALSDF, $ffvals)
      or die "Can't open output file $ffvals\n";
}
else {
   open($FFVALSDF, ">>$ffvals")
      or die "Can't open output file $ffvals\n";
}
open(ADDNBEST, "gzip -cqfd $addnbest |") or die "cannot open $addnbest to read\n";
open(ADDFFVALS, "gzip -cqfd $addffvals |") or die "cannot open $addffvals to read\n";

my $addedLine = 0;
while (defined($n = <ADDNBEST>) and defined($f = <ADDFFVALS>)) {
   chomp($n);
   chomp($f);
   if ( length($n) and ! exists $seen{$n}{$f} ) {
       ++$addedLine;
       $seen{$n}{$f} = 1;
       print($NBESTDF "$n\n");
       print($FFVALSDF "$f\n");
   }
}
die "FATAL ERROR: $addnbest is not of the same length as $addffvals\n"
   if (defined(<ADDNBEST>) or defined(<ADDFFVALS>));

close($NBESTDF);
close($FFVALSDF);
close(ADDNBEST);
close(ADDFFVALS);



print(STDERR "iniLine: $iniLine iniDup: $iniDup addedLine: $addedLine\n")
   if ($verbose > 1);

