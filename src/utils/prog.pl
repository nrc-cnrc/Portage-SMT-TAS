#!/usr/bin/env perl

# @file prog.pl 
# @brief Briefly describe your program here.
#
# @author Write your name here
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2020, Sa Majeste la Reine du Chef du Canada /
# Copyright 2020, Her Majesty in Right of Canada

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


# Synopsis:
# Display help when pgm called with -h/-help, and exit with 0 status:
#    usage
# For errors: output messages, show the help, and exit with non-zero status:
#    usage msg [msg2 [msg3]]
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
   exit @_ ? 1 : 0;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
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
) or usage "Error: Invalid option(s).";

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";

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

zopen(*IN, "<$in") or die "Error: Can't open $in for reading: $!\n";
zopen(*OUT, ">$out") or die "Error: Can't open $out for writing: $!\n";

binmode( IN,  ":encoding(UTF-8)" );
binmode( OUT, ":encoding(UTF-8)" );

while (<IN>) {
   print OUT;
}

close(IN);
close(OUT);

