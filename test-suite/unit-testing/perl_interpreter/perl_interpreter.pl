#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;

# @file test.pl 
# @brief Validates the weird way of calling Perl.
# 
# @author Samuel Larkin
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


our($help, $h);


if ($help or $h) {
   print "Help Message\n";
}
else {
   # Display the interpreter's version number.
   print $^V, "\n";
}
