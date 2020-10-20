#!/usr/bin/env python2
# @file summarize-canoe-results.py
# @brief Summarize the results of a set of Portage or PortageII training runs.
# 
# @author George Foster; updated by Darlene Stewart
#
# Includes experimentation with Python's class system. 
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, 2012 Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import sys, os, os.path, glob, re, math
import argparse
from argparse import ArgumentParser
from subprocess import check_output, CalledProcessError
from os.path import exists, join, isdir, basename, splitext

# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path).
if sys.argv[0] not in ('', '-c'):
    bin_path = os.path.dirname(sys.argv[0])
    if os.path.basename(bin_path) != "bin":
        sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

from portage_utils import *

def get_args():
   """Command line argument processing"""
   
   usage="summarize-canoe-results.py [options] [-dir] [dirs]"
   help="""
   Summarize the results of a set of canoe runs in the given list of directories,
   or in all immediate subdirectories if no list is given. Runs are sorted in
   order of descending average test-corpus score by default, but this can be
   changed with the -a option. Runs that are lacking one or more results included
   in the sort criterion are displayed last.
   """

   def is_dir(f):
      if not isdir(f):
         warn("Ignoring non-directory argument:", f)
         return ""
      return f

   parser = ArgumentParser(usage=usage, description=help, add_help=False)
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction, 
                       help="list directories that don't appear to contain a tuning run [%(default)s]")
   parser.add_argument("-d", "--debug", action=DebugAction)
   parser.add_argument("-e", dest="exclude_dirs", nargs="*", type=str, default="",
                       help="list of directories to exclude [%(default)s]")
   parser.add_argument("-t", dest="test_sets", nargs="*", type=str, default="",
                       help="list of test sets for which to list results, "
                            "e.g. 'test1 test2'; implies -a [include all tests]")
   parser.add_argument("-a", dest="sort_list", nargs="*", type=str, default="",
                       help="list of results to average for sorting, e.g. 'test1 test2'; "
                            "must be a subset of the listed test sets (-t option). "
                            "NB: scores in the 'avg' column are averages ONLY over these "
                            "results; this column isn't displayed if only one result is chosen "
                            "for sorting [average over all results specified by -t except MERT]")
   parser.add_argument("-g", dest="genre_suff", type=str, default="",
                       help="suffix to indicate results that are specific to genre or other characteristics; "
                       "if set to some value 'x', then scores will be displayed only for files of the "
                       "form *.bleu.x.* [look for *.bleu]")
   parser.add_argument("-delta", dest="delta", type=str, default=None,
                       help="also show delta BLEU score against a baseline [%(default)s]")
   parser.add_argument("-s", dest="alts", nargs="?", choices=("avg","trimmed", "max"), const="avg", default=None,
                       help="calculate stability stats over alternative runs stored in "
                       "sub-directories, when these exist: replace BLEU scores with averages, "
                       "and print std deviations on the following lines. Average can be "
                       "the arithmetic mean (avg, unspecified), trimmed mean (trimmed) "
                       "dropping the highest and lowest results for 4 or more runs, "
                       "or maximum (max) using the run with the maximum sorting average. [no]")
   parser.add_argument("-p", dest="prec", type=int, default=2,
                       help="number of decimal places for BLEU scores [%(default)s]")
   parser.add_argument("-dir", action='store_true', 
                       help="directories for which to summarize canoe results follow")
   parser.add_argument("dirs", nargs="*", type=is_dir, default=[f for f in os.listdir(".") if isdir(f)],
                       help="directories for which to summarize canoe results")
   cmd_args = parser.parse_args()
   
   excl = tuple(x for s in cmd_args.exclude_dirs for x in s.split())    # split any quoted strings
   cmd_args.dirs = tuple(d for d in cmd_args.dirs if d and d not in excl) # filter out excluded directories
   cmd_args.test_sets = tuple(t for s in cmd_args.test_sets for t in s.split())    # split any quoted strings
   cmd_args.sort_list = tuple(t for s in cmd_args.sort_list for t in s.split())    # split any quoted strings
   return cmd_args


devname = "MERT"


class DirInfo:
   """Maintain scoring info about a directory containing a tuning run."""
   test_sets = set()                    # all test set names
   sort_test_sets = []                  # list of test-set names to average for sorting
   avg_header = "avg"                   # header for averaged column
   delta_header = "delta"               # header for delta column
   cow_iter = ""                        # status of cow in iterations

   @staticmethod
   def initSortList(test_sets=None):
      """Initialize global sort_test_sets list from given test sets
      test_sets: an iterable providing the names of test sets
      """
      if test_sets is None or len(test_sets) is 0:
          DirInfo.sort_test_sets = [t for t in DirInfo.test_sets if t != devname]
          return
      for test_set in test_sets:
         if test_set in DirInfo.test_sets:
            DirInfo.sort_test_sets.append(test_set)
         else:
            warn("Ignoring non-existent or non-listed test set: ", test_set)

   def __init__(self, name, long_name=None):
      """Constructor.
      
      name: directory name
      long_name: long directory name for display purposes, typically the name
         from the command line arguments for the main run. Defaults to name.
      """
      self.name = name
      self.long_name = name if long_name is None else long_name
      self.cowlog_exists = False
      self.canoe_wts_exist = False
      self.scores = {}                 # one score per test set, or -1 for none
      # structures for handling alternative runs (used if cmd_args.alts is set)
      self.subdirs = []          # list of sub DirInfos
      self.avg_sdev_scores = {}  # 2-tuple of avg and sdev scores across main & alt runs for each test
      self.testavg_avg = 0       # avg of avgs across test sets (in sort_test_sets) for avgs across runs
      self.testavg_sdev = 0      # sdev of avgs across test sets (in sort_test_sets) for avgs across runs
      self.dir_max = None        # directory/sub-directory providing maximum average score

   def __str__(self):
      s = self.name + ": "
      for k in scores:
         if self.scores[k] is not None:
            s += " {0}={1}".format(k, self.scores[k])
      return s + " " + self.statusString()
   
   def getTestSets(self):
      """Return the names of test sets with scores in this DirInfo object."""
      return self.scores.keys()

   def getScore(self, test_set, average=False):
      """Return the score for a test_set or None if no score for the test set."""
      return self.scores.get(test_set) if not average else self.getAvg(test_set)

   def getAvg(self, test_set):
      """Return the average of the scores across the main and alt runs for a test_set or None."""
      return self.avg_sdev_scores.get(test_set, (None,None))[0]

   def getSDev(self, test_set):
      """Return the standard deviation of the scores across the main and alt runs for a test_set or None."""
      return self.avg_sdev_scores.get(test_set, (None,None))[1]

   def hasRun(self):
      return self.cowlog_exists or self.canoe_wts_exist

   def statusString(self):
      if self.cowlog_exists:
         msg = "" if self.canoe_wts_exist else "cow not finished"
      else:
         msg = "no local cow" if self.canoe_wts_exist else "cow not run"
      if self.cow_iter or msg:
         return "({0})".format("; ".join(s for s in (self.cow_iter, msg) if s))
      return ""

   def addScore(self, test_set, score):
      """Add a score for a test set."""
      DirInfo.test_sets.add(test_set)
      self.scores[test_set] = score

   def avgScore(self, avgOfAvgs=False):
      """Return the average across selected test sets.
      
      avgOfAvgs: return the average across the alternative run averages if True;
         otherwise return the average across the base scores
         or None if any scores are missing
      """
      if avgOfAvgs:
         return self.testavg_avg
      avg = 0.0
      for test_set in DirInfo.sort_test_sets:
         s = self.getScore(test_set, avgOfAvgs)
         if s is None: return None
         avg += s
      return avg / len(DirInfo.sort_test_sets) if len(DirInfo.sort_test_sets) else None

   def avgAndDevScore(self):
      """Return the averageScore across test sets and the dev score.
      Use the average for the run giving the maximum average if indicated by 
      self.dir_max; otherwise, if we have averages across alternative runs, use 
      those instead of the base scores.
      """
      dir = self.dir_max if self.dir_max else self
      use_avgs = len(self.avg_sdev_scores) > 0 and not self.dir_max
      return (dir.avgScore(use_avgs), self.getScore(devname, use_avgs))

   @staticmethod
   def colWidth(prec, header):
      return max(3 + prec, len(header))

   @staticmethod
   def printHeader(prec, printDelta):
      """Print column headings, assuming prec digits after the decimal point."""
      tests = sorted(DirInfo.test_sets)
      if len(DirInfo.sort_test_sets) > 1:
          tests.append(DirInfo.avg_header)
      if printDelta:
          tests.append(DirInfo.delta_header)
      for t in tests:
         print('{0:>{1}}  '.format(t, DirInfo.colWidth(prec, t)), end='')
      print()

   def prettyPrint(self, prec, baseline_score, avg):
      """Pretty print, with prec digits after the decimal point.
      
      prec: number of digits of precision after the decimal point.
      baseline_score: baseline_score for delta column, or None for no deltas.
      avg: if not None, print average scores for alternative runs instead of 
         base score. In this case, avg identifies the type of average computed
         (arithmetic mean ("avg") or trimmed average ("trimmed"), and a
         standard-deviation line is also printed.
      """
      def fmt_score(score, header):
          w = DirInfo.colWidth(prec, header)
          if score is not None:
             return '{0:{1}.{2}f}  '.format(score*100.0, w, prec)
          return '{0}  '.format('-' * w)

      show_avg = avg not in (None, "max")    # show average across alternative runs

      dir = self.dir_max if avg == "max" else self
      test_sets = sorted(DirInfo.test_sets)
      for test_set in test_sets:
         print(fmt_score(dir.getScore(test_set, show_avg), test_set), end='')
      if len(DirInfo.sort_test_sets) > 1:
         print(fmt_score(dir.avgScore(show_avg), DirInfo.avg_header), end='')
      if baseline_score is not None:
         avg_score = dir.avgScore(show_avg)
         delta = avg_score-baseline_score if avg_score is not None else None
         print(fmt_score(delta, DirInfo.delta_header), end='')
      print('', self.long_name if dir is self else join(self.long_name, dir.name), dir.statusString())

      # Print standard-deviation line below BLEU scores if printing averages
      # across alternative runs.
      if show_avg:
         for test_set in test_sets:
            print(fmt_score(self.getSDev(test_set), test_set), end='')
         if len(DirInfo.sort_test_sets) > 1:
            print(fmt_score(self.testavg_sdev, DirInfo.avg_header), end='')
         if baseline_score is not None:
            print(" " * (DirInfo.colWidth(prec, DirInfo.delta_header) + 2), end='')
         n = len(self.subdirs)
         runs = "{0}/{1}".format(n-1,n+1) if avg == "trimmed" and n > 3 else n+1
         print("    std devs over {0} run{1}".format(runs, "s" if len(self.subdirs) else ""))


def readDirInfo(di, d, in_test_sets_to_list, genre_suff):
   """Initialize a DirInfo object from the contents of a directory.
   
   di: DirInfo object to be initialized
   d: name of the directory to process
   in_test_sets_to_list(test_set): function returning true if the named test_set 
       should be included (i.e. listed in the results)
   genre_suff if non-empty, look for files of the form *.bleu.suff*
   """
   decode_dir = "decode"
   if di.name.startswith("translate"):
      variant = di.name[len("translate"):]
      if exists(join(d, "models", "decode" + variant)):
         decode_dir += variant
   if exists(join(d, "logs/log.cow")) or \
      exists(join(d, "log.cow")) or \
      exists(join(d, "models", decode_dir, "log.canoe.ini.cow")) or \
      exists(join(d, "logs/log.tune")):
      di.cowlog_exists = 1
   if exists(join(d, "canoe.ini.cow")) or exists(join(d, "models", decode_dir, "canoe.ini.cow")):
      di.canoe_wts_exist = 1
   summary_file = None
   if exists(join(d, "summary")):
      summary_file = "summary"
   elif exists(join(d, "models", decode_dir, "summary")):
      summary_file = join("models", decode_dir, "summary")
   if summary_file:    # tune.py run
      lines = list(open(join(d, summary_file)))
      scores = [float(l.split(None, 1)[0].split('=')[1]) for l in lines]
      if len(scores) != 0:
         best = max(scores)
         di.addScore(devname, best)
         di.cow_iter = "{0}/{1} iters".format(scores.index(best)+1, len(scores))
   else:
      rr_file = None
      if exists(join(d, "rescore-results")):
         rr_file = "rescore-results"
      elif exists(join(d, "models", decode_dir, "rescore-results")):
         rr_file = join("models", decode_dir, "rescore-results")
      if rr_file:
         s = check_output(["best-rr-val", join(d, rr_file)])
         devbleu = float(s.split(None, 1)[0])
         di.addScore(devname, devbleu)
         m = re.search("\((\d+) iters, best = (\d+)\)", s);
         di.cow_iter = "{0}/{1} iters".format(m.group(2), m.group(1))
   suff = "bleu."+genre_suff+"."
   globstr = "/*."+suff+"*" if genre_suff else "/*.bleu"
   for f in glob.glob(d + globstr) + glob.glob(d + "/translate" + globstr):
      if genre_suff:
         p = basename(f).find(suff)
         test = basename(f)[0:p] + basename(f)[p+len(suff):]
      else:
         test = splitext(basename(f))[0]
      if splitext(test)[1] == ".out":
         test = splitext(test)[0]
      if in_test_sets_to_list(test):
         try:
            toks = check_output(["grep", "BLEU", f]).split()
         except CalledProcessError:
            toks = []
         if len(toks) in (3, 5):
            di.addScore(test, float(toks[2]))
   return di


def avg_sdev(scores, mean_type="avg"):
   """Return average and standard deviation for an iterable.
   Note: the sdev formula is for the sample only (/n); not the one that estimates
   the population value (/(n-1)).
   
   scores: an iterable providing the scores to average
   mean_type: "trimmed" for trimmed mean (dropping the highest and lowest for 
      4 or more scores) or "avg" for arithmetic mean respectively.
      Default: avg
   """
   if len(scores) is 0:
       return None, None
   if mean_type == "trimmed" and len(scores) > 3:
       min_score = min(scores)
       max_score = max(scores)
       min_max = min_score + max_score
       min_max_sq = min_score * min_score + max_score * max_score
       n = len(scores) - 2
   else:
       min_max = min_max_sq = 0.0
       n = len(scores)
   avg = (sum(scores) - min_max) / n
   sdev_sq = (sum(s * s for s in scores) - min_max_sq) / n - avg * avg
   sdev = math.sqrt(sdev_sq) if sdev_sq > 0.0 else 0.0
   return avg, sdev

def main():
   printCopyright("summarize-canoe-results.py", 2010);
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';
   
   cmd_args = get_args()
   
   def in_test_sets_to_list(test_set):
      """Return true if the named test_set should be listed in the results."""
      return len(cmd_args.test_sets) is 0 or test_set in cmd_args.test_sets

   # Extract results from each directory and make a list of them
   results = []
   for d in cmd_args.dirs:
      di = DirInfo(basename(d), long_name=d)
      readDirInfo(di, d, in_test_sets_to_list, cmd_args.genre_suff)
      results.append(di)
      # check for sub-dirs containing alternative runs if called for
      if cmd_args.alts:
         for sd in os.listdir(d):
            sdir = join(d, sd)
            # Note: translate subdir was already processed by readDirInfo(di) above
            if sd not in ("translate", "foos", "logs") and isdir(sdir):
               sdi = DirInfo(sd)
               readDirInfo(sdi, sdir, in_test_sets_to_list, cmd_args.genre_suff)
               if sdi.hasRun():
                  di.subdirs.append(sdi)
   
   # Make list of tests to average for sorting
   DirInfo.initSortList(cmd_args.sort_list)
   
   # Fill in average and sdev for each test set from alts and the sdev of the 
   # averages across test sets, if called for.
   # The sdev formula is for the sample only (/n); not the one that
   # estimates the population value (/(n-1)).
   if cmd_args.alts:
      for di in results:
         if cmd_args.alts == "max":
            # Determine which alternative run gives the maximum average score
            scores = [di] + di.subdirs
            i_max = scores.index(max(scores, key=DirInfo.avgScore))
            di.dir_max = di if i_max is 0 else di.subdirs[i_max-1]
         else:
            if cmd_args.alts == "trimmed" and len(di.subdirs) < 3:
               warn("Reverting to arithmetic mean for < 4 runs in", di.name)
            for test in di.getTestSets():
               scores = [s for s in (di.getScore(test),) if s is not None]
               for sdi in di.subdirs:
                  sc = sdi.getScore(test)
                  scores.append(sc) if sc is not None \
                     else warn("Alt value missing for", test, "in", join(di.name, sdi.name))
               di.avg_sdev_scores[test] = avg_sdev(scores, cmd_args.alts)
            scores = [s for s in (di.avgScore(),) if s is not None]
            scores.extend(s for sdi in di.subdirs for s in (sdi.avgScore(),) if s is not None)
            di.testavg_avg, di.testavg_sdev = avg_sdev(scores, cmd_args.alts)
   
   # sort results by the sorting average
   results.sort(key=DirInfo.avgAndDevScore, reverse=True)
   
   # output sorted results according to chosen format
   if len(DirInfo.test_sets) is 0:
      info("No BLEU results found in subdirectories.")
      sys.exit(0)
   
   # What is our baseline average score?
   baseline_score = None
   if cmd_args.delta:
      for r in results: 
         if r.name == cmd_args.delta:
            baseline_score = r.avgScore(cmd_args.alts != None)
            if cmd_args.verbose:
               print("baseline:", baseline_score)
            break

   DirInfo.printHeader(cmd_args.prec, baseline_score is not None)
   for r in results:
      if cmd_args.verbose or r.hasRun():
         r.prettyPrint(cmd_args.prec, baseline_score, cmd_args.alts)
   return


if __name__ == '__main__':
    main()
