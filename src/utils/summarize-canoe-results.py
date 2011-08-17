#!/usr/bin/env python
# @file summarize-canoe-results.py
# $Id$
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

import sys, os, os.path, glob, re
from optparse import OptionParser
from subprocess import Popen, PIPE
from os.path import exists, join, isdir, basename, splitext

# arg processing

usage="summarize-canoe-results.py [options] [dirs]"
help="""

Summarize the results of a set of canoe runs in the given list of directories,
or in all immediate subdirectories if no list is given. Runs are sorted in
order of descending average test-corpus score by default, but this can be
changed with the -a and -d options. Runs that are lacking one or more results
included in the sort criterion are displayed last.
"""

parser = OptionParser(usage=usage, description=help)
parser.add_option("-v", dest="verbose", action="store_true", default=False,
                  help="list directories that don't appear to contain a cow run [%default]")
parser.add_option("-a", dest="sort_list", type="string", default="",
                  help="list of results to average for sorting, eg 'test1 test2'\
                  (NB: no explicit 'avg' column is displayed if only one result\
                  is chosen for sorting.) [average over all except mert]")
parser.add_option("-e", dest="excludedirs", type="string", default="",
                  help="list of directories to exclude [%default]")
parser.add_option("-p", dest="prec", type="int", default=2,
                  help="number of decimal places for BLEU scores [%default]")
(opts, args) = parser.parse_args()

if len(args):                           # explicit directory list given
   dirs = filter(lambda f: isdir(f), args)
   if len(dirs) != len(args):
      print >> sys.stderr, "Warning: ignoring non-directory arguments: ", [f for f in set(args)-set(dirs)]
else:                                   # no dirs given: use all subdirs
   dirs = filter(lambda f: isdir(f), os.listdir("."))

excl = opts.excludedirs.split()
dirs = filter(lambda f: f not in excl, dirs)

# objects

class DirInfo:
   """Maintain scoring info about a directory containing a cow run."""

   test_sets = {}                       # test set name -> index
   sort_indexes = []                    # list of test-set indexes to average for sorting
   avg_header = "avg"                   # header for averaged column
   cow_iter = ""                        # status of cow in iterations

   @staticmethod
   def initSortIndexes(tests):
      """Initialize global sort_indexes[] list from given space-separated list of test sets"""
      for t in tests.split():
         if t in DirInfo.test_sets:
            DirInfo.sort_indexes.append(DirInfo.test_sets[t])
         else:
            print >> sys.stderr, "ignoring non-existent test set: " + t

   def __init__(self, name):
      self.name = name
      self.cowlog_exists = False
      self.canoe_wts_exist = False
      self.scores = []                 # one score per test set, or -1 for none

   def __str__(self):
      s = self.name + ":"
      for k,v in DirInfo.test_sets.iteritems():
         if self.getScore(v) != -1:
            s += " " + k + "=" + str(self.scores[v])
      return s + " " + self.statusString()

   def getScore(self, index):
      return self.scores[index] if index < len(self.scores) else -1

   def hasRun(self):
      return self.cowlog_exists or self.canoe_wts_exist

   def statusString(self):
      if self.cow_iter != "":
         cow_iter_format = self.cow_iter + "; "
      else:
         cow_iter_format = ""
      if self.cowlog_exists and self.canoe_wts_exist:
         if self.cow_iter != "":
            return "(" + self.cow_iter + ")"
         else:
            return ""
      elif self.cowlog_exists and not self.canoe_wts_exist:
         return "(" + cow_iter_format + "cow not finished)"
      elif not self.cowlog_exists and self.canoe_wts_exist:
         return "(" + cow_iter_format + "no local cow)"
      else:
         return "(" + cow_iter_format + "cow not run)"

   def addScore(self, testset, score):
      if testset not in DirInfo.test_sets:
         DirInfo.test_sets[testset] = len(DirInfo.test_sets)
      i = DirInfo.test_sets[testset]
      self.scores.extend([-1] * (i + 1 - len(self.scores)))
      self.scores[i] = score

   def avgScore(self):
      avg = 0.0
      for i in DirInfo.sort_indexes:
         s = self.getScore(i)
         if s == -1: return -1
         avg += s
      return avg / len(DirInfo.sort_indexes) if len(DirInfo.sort_indexes) else -1

   def avgAndDevScore(self):
      avg = self.avgScore()
      dev = self.getScore(0)
      return [avg, dev]

   @staticmethod
   def colWidth(prec, header_len):
      return max(3 + prec, header_len)

   @staticmethod
   def printHeader(prec):
      """Print column headings, assuming prec digits after the decimal point."""
      tests = DirInfo.test_sets.keys()
      tests.sort()
      for t in tests:
         print t.rjust(DirInfo.colWidth(prec, len(t))), "",
      if len(DirInfo.sort_indexes) > 1:
         t = DirInfo.avg_header
         print t.rjust(DirInfo.colWidth(prec, len(t))), "",
      print

   @staticmethod
   def printScore(prec, header_len, score):
      w = DirInfo.colWidth(prec, header_len)
      fmt = "%" + str(w) + "." + str(prec) + "f"
      print fmt % (score * 100.0) if score != -1 else ("-" * w), "",

   def prettyPrint(self, prec):
      """Pretty print, with prec digits after the decimal point."""
      tests = DirInfo.test_sets.keys()
      tests.sort()
      for t in tests:
         DirInfo.printScore(prec, len(t), self.getScore(DirInfo.test_sets[t]))
      if len(DirInfo.sort_indexes) > 1:
         DirInfo.printScore(prec, len(DirInfo.avg_header), self.avgScore())
      print "", self.name, self.statusString()

# extract results from each subdir and make list of all results

devname = "MERT"

results = []
     
for d in dirs:
   di = DirInfo(basename(d))

   if exists(join(d, "logs/log.cow")) or \
      exists(join(d, "log.cow")) or \
      exists(join(d, "models/decode/log.canoe.ini.cow")) or \
      exists(join(d, "logs/log.tune")):
      di.cowlog_exists = 1
   if exists(join(d, "canoe.ini.cow")) or exists(join(d, "models/decode/canoe.ini.cow")):
      di.canoe_wts_exist = 1
   rr_file = ""
   if exists(join(d, "rescore-results")):
      rr_file = "rescore-results"
   elif exists(join(d, "models/decode/rescore-results")):
      rr_file = "models/decode/rescore-results"
   if rr_file != "" and not exists(join(d, "summary")):
      s = Popen(["best-rr-val", join(d,rr_file)], stdout=PIPE).communicate()[0]
      devbleu = float(s.split()[0])
      di.addScore(devname, devbleu)
      m = re.search("\((\d+) iters, best = (\d+)\)", s);
      di.cow_iter = s[m.start(2):m.end(2)] + "/" + s[m.start(1):m.end(1)] + " iters"
   elif exists(join(d, "summary")):   # tune.py run
      lines = list(open(join(d, "summary")))
      scores = map(float, [l.split()[0].split('=')[1] for l in lines])
      if len(scores) != 0:
         best = max(scores)
         di.addScore(devname, best)
         di.cow_iter = str(scores.index(best)+1) + "/" + str(len(scores)) + " iters"

   for f in glob.glob(d + "/*.bleu") + glob.glob(d + "/translate/*.bleu"):
      test = splitext(basename(f))[0]
      if splitext(test)[1] == ".out":
         test = splitext(test)[0]
      toks = Popen(["grep", "BLEU", f], stdout=PIPE).communicate()[0].split()
      if len(toks) == 3 or len(toks) == 5:
         di.addScore(test, float(toks[2]))

   results.append(di)

# make list of test-corpus indexes to average for sorting, and sort results by
# this average

if opts.sort_list == "":
   for k in DirInfo.test_sets:
      if k != devname:
         opts.sort_list += k + " "
DirInfo.initSortIndexes(opts.sort_list)

results.sort(key=DirInfo.avgAndDevScore, reverse=True)

# output sorted results according to chosen format

if len(DirInfo.test_sets) == 0:
   print >> sys.stderr, "no BLEU results found in subdirectories"
   sys.exit(0)

DirInfo.printHeader(opts.prec)
for r in results:
   if opts.verbose or r.hasRun():
      r.prettyPrint(opts.prec)
