#!/usr/bin/env python
#
# @author Nicola Ueffing
# @file reverse_joint_phr_tab.py 
# @brief Reverse a joint phrase table or a phrase table with only one
# probability column.
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

import string, sys

help = """
reverse_joint_phr_tab.py < input > output

  Reverse a JPT.

"""

if (len(sys.argv) > 1 and sys.argv[1] == "-h") or len(sys.argv) > 3:
   sys.stderr.write(help);
   sys.exit(1)

line = sys.stdin.readline()
while line != "":
    l = string.split(string.strip(line), " ||| ")
    assert(len(l) == 3)
    sys.stdout.write("%s ||| %s ||| %s\n" % (l[1], l[0], l[2]))
    line = sys.stdin.readline()
