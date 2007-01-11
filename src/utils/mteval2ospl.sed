#!/bin/sed -f
# Sed script for converting from SGM format to one-sentence-per-line

# Used as follows:
# mteval2ospl.sed myfile.sgm > myfile.ospl

# Get rid of all non-<seg> lines
/^ *<seg.*>.*<\/seg>.*$/! d

# Strip <seg> tag and whitespace from start
s/^ *<seg[^>]*> *//

# Strip <seg> tag and whitespace from end
s/ *<\/seg>.*$//
