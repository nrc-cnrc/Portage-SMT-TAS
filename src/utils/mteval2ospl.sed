#!/bin/sed -f
# @file mteval2ospl.sed
# @brief Sed script for converting from SGM format to one-sentence-per-line.
# 
# @author George Foster
# 
# COMMENTS: 
#
# George Foster
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# Copyright 2006, Her Majesty in Right of Canada


# Used as follows:
# mteval2ospl.sed myfile.sgm > myfile.ospl

# Get rid of all non-<seg> lines
/^ *<seg.*>.*<\/seg>.*$/! d

# Strip <seg> tag and whitespace from start
s/^ *<seg[^>]*> *//

# Strip <seg> tag and whitespace from end
s/ *<\/seg>.*$//
