#!/usr/bin/perl
# $Id$

# prog.pl Program template
#
# PROGRAMMER: George Foster
#
# COMMENTS:
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "prog.pl, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada\n";

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [-f(lag)] [-opt_with_string_arg <arg>]
       [-opt_with_integer_arg <arg>] [-opt_with_float_arg <arg>] [-h(elp)]
       [-v(erbose)] [in [out]]

  Do something...

Options:

  -f(lag):      ...

  -opt_with_string_arg <arg>: set ... to ...

  -opt_with_integer_arg <N>: set ... to ...

  -opt_with_float_arg <Value>: set ... to ...

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
