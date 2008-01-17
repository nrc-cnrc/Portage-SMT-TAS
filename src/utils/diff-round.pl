#!/usr/bin/perl
# $Id$
#
# diff-round.pl - diff two ff files, i.e., files of floating point numbers, where
#                 rounding differences are not real differences
#
# PROGRAMMER: Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "diff-round.pl, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada\n";

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: diff-round.pl [-h(elp)] [-prec P] infile1 infile2

  Assuming infile1 and infile2 are files of numbers with the same layout,
  compare them ignoring differences past the P'th significant digit.
  More precisely, ignore differences where |a-b| < max(|a|,|b|) / 10^P.

Notes:
  To compare two phrase tables that contain the same phrase pairs:
     LC_ALL=C diff-round.pl 'sort pt1 |' 'sort pt2 |'
     LC_ALL=C diff-round.pl 'gzip -cqfd pt1 | sort |' 'gzip -cqfd pt2 | sort |'

Options:
  -prec P       precision to retain before comparing [6]
  -h(elp):      print this help message
";
   exit 1;
}

use Getopt::Long;
my $prec = 6;
GetOptions(
   help         => sub { usage },
   "prec=i"     => \$prec,
) or usage;
my $pow_prec = 1/(10**$prec);

2 == @ARGV or usage "Must specify exactly two input files.";

# Will hold the maximum numerical difference found
my $max_diff = 0;

open F1, $ARGV[0] or die "Can't open $ARGV[0]: $!";
open F2, $ARGV[1] or die "Can't open $ARGV[1]: $!";

sub max($$) {
   $_[0] < $_[1] ? $_[1] : $_[0];
}

# diff_epsilon($a, $b) returns true iff $a and $b differ by more than their
# $prec'th significant digit.
sub diff_epsilon ($$) {
   return 0 if $_[0] eq $_[1];
   {
      no warnings;
      return 1 if $_[0] + 0 ne $_[0];
      return 1 if $_[1] + 0 ne $_[1];
   }
   my $max = max(abs($_[0]), abs($_[1]));
   if ( $max > 0 ) {
      my $rel_diff = abs($_[0] - $_[1]) / $max;
      $max_diff = $rel_diff if ($rel_diff > $max_diff);
      return ($rel_diff > $pow_prec);
   } else {
      return 0;
   }
}

while (<F1>) {
   my $L1 = $_; chomp $L1;
   my $L2 = <F2>; chomp $L2 if defined $L2;
   die "Unexpected end of $ARGV[1] before end of $ARGV[0] at line $.\n"
      unless defined $L2;

   # Optimization: don't do the fancy (and expensive) math if the lines are
   # identical
   next if $L1 eq $L2;

   # Split each line into space separated tokens
   my @L1 = split /\s+/, $L1;
   my @L2 = split /\s+/, $L2;

   if ( $#L1 != $#L2 ) {
      print "$. << $L1   >> $L2\n";
   } else {
      for my $i (0 .. $#L1) {
         if ( diff_epsilon($L1[$i], $L2[$i]) ) {
            print $., ($#L1 > 0 ? "($i)" : ""), " < $L1[$i]   > $L2[$i]\n";
         }
      }
   }

   #if ( abs($L1 - $L2) * 10**$prec > max(abs($L1), abs($L2)) ) {
   #   print "$.	< $L1	> $L2\n"
   #}
}
die "Unexpected end of $ARGV[0] before end of $ARGV[1] at line $.\n"
   unless eof(F2);

print STDERR "Maximum relative numerical difference: $max_diff\n";
