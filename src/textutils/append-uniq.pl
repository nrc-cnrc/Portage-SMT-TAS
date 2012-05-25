#!/usr/bin/env perl
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
printCopyright "append-uniq.pl", 2005;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [-h(elp)] [-v(erbose)] -i index -m modulo -S number_of_NBest
       -nbest=file_pattern -ffvals=file_pattern
       -addnbest=file_pattern -addffvals=file_pattern

   Appends non duplicate lines from addnbest_pattern and addffvals_pattern
   files to nbest_pattern and ffvals_pattern, where a duplicate is a line which
   is identical to another in *both* files at the same time.
   
   You must provide an index i, a modulo/step m and S and this script with apply
   the patterns using values comparable to running seq i m S.

   Nbest and ffvals must be duplicate free, which you can accomplish using
   the following command:
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
   "nbest=s"     => \my $nbest_pattern,
   "addnbest=s"  => \my $addnbest_pattern,
   "ffvals=s"    => \my $ffvals_pattern,
   "addffvals=s" => \my $addffvals_pattern,
   "i=i"         => \my $index,
   "m=i"         => \my $modulo,
   "S=i"         => \my $S,

   help          => sub { usage },
   verbose       => sub { ++$verbose },
   quiet         => sub { $verbose = 0 },
   debug         => \my $debug,
) or usage;

if ( $debug ) {
   no warnings;
   print STDERR "
   nbest       = $nbest_pattern
   ffvals      = $ffvals_pattern
   addnbest    = $addnbest_pattern
   addffvals   = $addffvals_pattern
   i           = $index
   m           = $modulo
   S           = $S
   verbose     = $verbose
   debug       = $debug

";
}

die "You must define i" unless (defined($index));
die "You must define m" unless (defined($modulo));
die "You must define S" unless (defined($S));

sub process($$$$) {
   my ($nbest, $ffvals, $addnbest, $addffvals) = @_;
   print STDERR "Appending $nbest, $ffvals, $addnbest, $addffvals\n" if ($debug);
   for ( $nbest, $ffvals, $addnbest, $addffvals ) {
      defined $_ or die "append-uniq.pl: -nbest, -ffvals, -addnbest and -addffvals are all required.\n";
      -r $_ or die "append-uniq.pl: Can't open $_ for reading: $!\n";
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
   print STDOUT "$iniLine + $addedLine = " . ($iniLine + $addedLine) . "\n";
}

for (my $i=$index; $i<$S; $i+=$modulo) {
   process(sprintf($nbest_pattern, $i), sprintf($ffvals_pattern, $i), sprintf($addnbest_pattern, $i), sprintf($addffvals_pattern, $i));
}


