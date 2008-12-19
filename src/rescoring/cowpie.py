#!/usr/bin/env python

# @file cowpie.py
# @brief Analyze a cow log file for Powell and other data, and print a report.
# 
# @author George Foster
# 
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

import sys
import math

# Arg processing

sys.stderr.write("cowpie.py, NRC-CNRC, (c) 2008, Her Majesty in Right of Canada\n")

help = """
cowpie.py [log.cow]

Analyze a cow log file for Powell and other data, and print a report. Columns
are, left to right:
  iter        - iteration index
  time        - total decoding time in seconds for this iter
  bleu        - post-decoding bleu score
  bleu-cal    - difference between decoding bleu and prev Powell bleu
  wtdev       - std dev for weight vector, after normalizing by avg abs weight
  wtdist      - squared distance between current weight vector and previous
  nb size     - cumulative total size of nbest lists
  time        - total Powell time in seconds for this iter
  bleu        - Powell bleu score
  best/ntries - index of best Powell try / number of tries
  choice      - index of chosen Powell try - may differ from best if -chw is used

Options:

-f  Don't print headers and don't format columns; just write space-separated
    list of numbers, for easy use with cut.

"""

if (len(sys.argv) > 1 and sys.argv[1] == "-h") or len(sys.argv) > 3:
   sys.stderr.write(help);
   sys.exit(1)

no_format = 0
if len(sys.argv) > 1 and sys.argv[1] == "-f":
   no_format = 1
   sys.argv.pop(1)

if len(sys.argv) == 2: 
   file = open(sys.argv[1])
else: 
   file = sys.stdin

# Helpers

def argmax(a):
   """Return (index,val) for the largest element in a"""
   besti = -1
   for i in range(len(a)):
      if besti == -1 or a[i] > a[besti]:
         besti = i
   return besti, a[besti]

def addabs(x, y): return abs(x) + abs(y)

def norm_sdev(a):
   """Compute std deviation for elements in a, after normalizing with mean"""
   mean = reduce(addabs, a) / len(a)
   s = 0
   for x in a:
      ss = x / mean - 1.0
      s += ss * ss
   return math.sqrt(s / len(a))

def wt_dist(a, b):
   """Compute squared distance between wts a and b"""
   sum = 0
   mlen = min(len(a), len(b))
   for i in range(mlen):
       diff = a[i] - b[i]
       sum += diff * diff
   return math.sqrt(sum)

# Enter the pie

prev_wts = []
score_stats = []
iter = 0
decodetime = 0
oldpowbleu = 0
choosing = False
choicescore = 0
choice = 0

if no_format == 0:
   print "%-4s %s%s" % ("", "[----------------- DECODING -----------------]", "[----------- POWELL -----------]")
   print "%4s %5s %7s %8s %6s %6s %8s %5s %7s %s %s" % \
      ("iter", "time", "bleu", "bleu-cal", "wtdev", "wtdist", "nb size", "time", "bleu", "best/ntries", "choice")

for line in file:

   # Translating n sentences..Translated n sentences in s seconds.
   if line.startswith("Translating"):
      toks = line.split()
      decodetime += int(toks[6])

   # Current weight vector: -d 1 -w 0 -lm 1:1 -tm 1:1:1:1:1:1:1:1 -ftm 1:1:1:1:1:1:1:1
   elif line.startswith("Current weight vector:"):
      wts = []
      toks = line.split()
      del(toks[0:3])
      for t in [toks[i] for i in range(1, len(toks), 2)]:
         for w in t.split(':'):
            wts.append(float(w))
      wtdev = norm_sdev(wts)
      wtdist = wt_dist(wts, prev_wts)
      prev_wts = wts

   # BLEU score: b
   elif line.startswith("BLEU score:"):
      toks = line.split()
      decodebleu = 100 * float(toks[2])

   # No new sentences in the N-best lists.
   # Reached maximum number of iterations. Stopping.
   # Too many iterations since last maximum found - quitting
   elif line.startswith("No new") or line.startswith("Reached maximum") or line.startswith("Too many"):
      iter += 1
      if no_format == 0:
         print "%4d %5d %7.4f %8.4f %6.3f %6.3f %8d   no final powell step" % \
            (iter, decodetime, decodebleu, decodebleu-oldpowbleu, wtdev, wtdist, nbsize)
      else:
         print iter, decodetime, decodebleu, decodebleu-oldpowbleu, wtdev, wtdist, nbsize, \
               0, 0, 0, 0
      sys.exit(0)

   # Total size of n-best list -- previous: oldsize; current: size.
   elif line.startswith("Total size of"):
      toks = line.split()
      nbsize = int(toks[9][:-1])

   # Score: -L <=> S in T seconds
   elif line.startswith("Score: ") and line.find("seconds") != -1:
      toks = line.split()
      if (len(toks) == 7):         # old format:   Score: -2.029859 <=> 0.131354 in 0 seconds
         score_stats.append((toks[3], toks[5]))
      else:                        # new format:   Score: 0.131354 in 0 seconds
         score_stats.append((toks[1], toks[3]))
         
   # choosing new best weights using max-score / distance-from-best criterion:
   elif line.startswith("choosing new best"):
      choosing = True

   # 2 0.773712 = 0.75*0.985093 + 0.25*0.13957
   elif choosing and line.find("*") != -1 and line.find(" = ") != -1:
      toks = line.split()
      if float(toks[1]) > choicescore:
         choice = int(toks[0]) + 1
         choicescore = float(toks[1])

   # Best score: 0.229231
   elif line.startswith("Best score: "):
      iter += 1
      powtime = sum([int(x[1]) for x in score_stats])
      besti, best = argmax([float(x[0]) for x in score_stats])
      powbestiter = besti+1
      powbleu = 100 * best
      if not choosing: choice = powbestiter
      
      if no_format == 0: 
         print "%4d %5d %7.4f %8.4f %6.3f %6.3f %8d %5d %7.4f %4d/%-6d %4d" % \
                   (iter, decodetime, decodebleu, decodebleu-oldpowbleu, wtdev, wtdist, nbsize, \
                   powtime, powbleu, powbestiter, len(score_stats), choice)
      else: 
         print iter, decodetime, decodebleu, decodebleu-oldpowbleu, wtdev, wtdist, nbsize, \
               powtime, powbleu, powbestiter, len(score_stats), choice

      score_stats = []
      decodetime = 0
      oldpowbleu = powbleu
      choicescore = 0

