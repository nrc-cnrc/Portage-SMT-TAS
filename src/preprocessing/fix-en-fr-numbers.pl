#!/usr/bin/perl
# $Id$

# @file fix-en-fr-numbers.pl
# @brief post-processing script to reformat English-style numbers for French.
#
# This script is intended to be used after detokenization, e.g., in
# postprocess_plugin.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

use strict;
use warnings;

if ( $#ARGV > 0 && ($ARGV[0] eq "-h" || $ARGV[0] eq "-help" ) ) {
   print STDERR "
fix-en-fr-numbers.pl [INPUT FILE(S)]

  Post-process the output of English->French translation to reformat English-
  style numbers using French-style rules: 10%->10 %; 1,000.00->1 000,00; etc.
  Intended for use after detokenization, e.g., in postprocess_plugin.
";
   exit(1);
}

# Don't specify encoding, since the stuff that is handled here is all ASCII,
# so that an encoding agnostic treatment will make it work with latin1 as well
# as utf8.
#binmode STDIN,  ":encoding(utf-8)";
#binmode STDOUT, ":encoding(utf-8)";

# NOTE: Usually the order of substitions is important.
while(<>) {
   # Add a space between digit %
   s/([0-9])%/$1 %/g;

   # This one rule replaces the other ones for command to space conversion,
   # and handles more cases correctly, in particular 0.000,02 -> 0.000 02
   # The period is *not* replaced here, even though it would be simple to do
   # so, because it introduces errors, which Uli's original RE doesn't.
   s/[0-9]{1,3}(?:,[0-9]{3})*(?:\.(?:[0-9]{3},)*[0-9]{1,3})?/
     { my $num = $&; $num =~ tr#,# #; $num }
    /exg;


   # This rule removed - the case is already correctly handled by the rest of
   # the code, while it introduces errors, as detected during unit testing.
   # fix 1,000.00 => 1 000,00
   #s/([0-9]),([0-9]{3})\./$1 $2,/g;

   # fix 1,000 => 1 000
   #s/([0-9]),(?=[0-9]{3})/$1 /g;

   #s/([^\.][0-9]+)\.([0-9])/$1,$2/g;

   #s/^([0-9]+)\.([0-9]+[^ \.])/$1,$2/;

   # Replace the decimal . by , but be careful not to modify cases with slashes
   # or more than one ., such as section numbers (e.g., 1.3.5).
   # We will still incorectly change section numbers such as 1.3 to 1,3, but
   # avoiding those would require logic well beyond the intent of the simple
   # hack implemented in this script.
   # REGEX by Ulrich Germann
   s/(^|[^0-9\.\/])([0-9]+)\.([0-9]+(?!\.[0-9]))/$1$2,$3/gi;

   print;
}
