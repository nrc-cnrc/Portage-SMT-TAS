#!/bin/sh
# $Id$
#
# which-test.sh - portable wrapper for which, with reliable status code
#
# Eric Joanis
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

# Usage: which-test.sh prog
# Exists with status code 0 if prog is on the path and executable, 1 otherwise.
# Example use in a bash or sh script:
#    if which-test.sh prog; then
#       # prog is available; you can use it
#    else
#       # prog doesn't exist, isn't on the path, or isn't executable;
#       # work around it or give up
#    fi

# Motivation for this script: on some Solaris and Mac installations, "which"
# does not return a fail status code when the program doesn't exist.  This test
# here has been tested on Solaris, Mac OS X, and various Linux distros.

if [ -x "`which $1 2> /dev/null`" ]; then
   exit 0
else
   exit 1
fi
