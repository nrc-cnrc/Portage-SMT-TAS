#!/usr/bin/env perl
# $Id$

# @file prog.pl 
# @brief Briefly describe your program here.
#
# @author Write your name here
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

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
printCopyright "prog.pl", 2006;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [IN [OUT]]

  Briefly describe what your program does here

Options:

  -f(lag)       set some flag
  -opt_with_string_arg ARG  set ... to ...
  -opt_with_integer_arg N   set ... to ...
  -opt_with_float_arg VAL   set ... to ...
  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -d(ebug)      print debugging information
";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
   flag        => \my $flag,
   "opt_with_string_arg=s"  => \my $opt_with_string_arg_value,
   "opt_with_integer_arg=i" => \my $opt_with_integer_arg_value,
   "opt_with_float_arg=f"   => \my $opt_with_float_arg_value,
) or usage;

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

$verbose and print STDERR "Mildly verbose output\n";
$verbose > 1 and print STDERR "Very verbose output\n";
$verbose > 2 and print STDERR "Exceedingly verbose output\n";

if ( $debug ) {
   no warnings;
   print STDERR "
   in          = $in
   out         = $out
   verbose     = $verbose
   debug       = $debug
   flag        = $flag
   opt_with_string_arg_value  = $opt_with_string_arg_value
   opt_with_integer_arg_value = $opt_with_integer_arg_value
   opt_with_float_arg_value   = $opt_with_float_arg_value

";
}

open(IN, "<$in") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

while (<IN>) {
   print OUT;
}
