#!/usr/bin/env python
#
# @author Nicola Ueffing
# @file reverse_cond_phr_tab.py 
# @brief Reverse a multi-prob conditional phrase table.
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2007, Her Majesty in Right of Canada

import string, sys

help = """
reverse_cond_phr_tab.py < input > output

  Reverse a multi-prob CPT with the same number of forward and backward models.
  4th column and end-of-3rd column data (e.g., a= or c= info) are not allowed.

"""

if (len(sys.argv) > 1 and sys.argv[1] == "-h") or len(sys.argv) > 3:
   sys.stderr.write(help);
   sys.exit(1)

line = sys.stdin.readline()
while line != "":
   l = string.split(string.strip(line), " ||| ")
   assert(len(l) == 3)
   probs  = string.split(string.strip(l[2]))
   if (len(probs)%2 == 1) and (probs[-1][:2] == "c="):
      count = " " + probs[-1]
      probs[-1:] = []
   else:
      count = ""
   assert(len(probs)%2 == 0)
   fprobs = ""
   bprobs = ""    
   for i in range(0, len(probs)/2):
      fprobs += " " + probs[i]
   for i in range(len(probs)/2, len(probs)):
      bprobs += " " + probs[i]
   sys.stdout.write("%s ||| %s |||%s%s%s\n" % (l[1], l[0], bprobs, fprobs, count))
   line = sys.stdin.readline()
