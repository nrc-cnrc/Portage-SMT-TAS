#!/bin/bash
# $Id$

# @file clean.sh
# @brief Clean up for r-parallel-d.pl's unittest.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada



rm -f log.debug
find . -type d -name run-p.\* | xargs -r rm -r

exit
