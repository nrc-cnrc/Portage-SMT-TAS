#!/usr/bin/env python

# @file sri-mix-lms.py
# @brief Create a mixture of LMs using SRILM's ngram command.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import
import sys
import os.path
from argparse import ArgumentParser, FileType
import codecs
import tempfile
from subprocess import call, check_output, CalledProcessError
from portage_utils import *

def get_args():
   """Command line argument processing."""
   
   usage="sri-mix-lms.py [options] cmpts wts outlm"
   help="""
   Create a mixture of input LMs with given weights using SRILM's ngram command.
   This is an approximation with no run-time cost, unlike Portage's mixlm, which
   computes exact mixtures at runtime. This program lets you bypass SRILM's
   awkward syntax and limitation to 10 models to be interpolated. It trains
   sub-models sequentially, so won't be very efficient for very large mixtures.
   """

   parser = ArgumentParser(usage=usage, description=help, add_help=False)
   parser.add_argument("cmpts", type=open, help="file containing pathnames of LMs to mix")
   parser.add_argument("wts", type=open, help="file containing one weight per LM to mix")
   parser.add_argument("outlm", type=str, help="output LM")
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-e", dest="exten", type=str, default="",
                       help="suffix to append to each path in cmpts")
   parser.add_argument("-f", dest="force", action='store_true', 
                       help="overwrite outlm if it exists")
   return parser.parse_args()

def get_order(lm):
   """Return the order of a given SRILM LM."""

   order = 0
   for line in open(lm):
      if line.startswith("ngram "):
         try: n = int(line[len("ngram "):].split('=')[0])
         except: n = 0
         if n > order: order = n
      if order and line.startswith("\\"):
         break
   return order

def train_mixlm(cmpts, wts, order, out):
   """Train a mixture of given weighted component models (max 10)."""

   assert 1 <= len(cmpts) <= 10
   assert len(wts) == len(cmpts)
   cmd = ["ngram", "-order", str(order)]
   for i in range(0, len(cmpts)):
      if i == 0: cmd.extend(["-lm", cmpts[i], "-lambda", str(wts[i])])
      elif i == 1: cmd.extend(["-mix-lm", cmpts[i]])
      else: cmd.extend(["-mix-lm" + str(i), cmpts[i], "-mix-lambda" + str(i), str(wts[i])])
   cmd.extend(["-write-lm", out])
   return call(cmd)
   
def main():
   printCopyright("prog.py", 2013);
   args = get_args()

   cmpts = [s + args.exten for s in args.cmpts.read().splitlines()]
   wts = [float(a) for a in args.wts.read().splitlines()]

   if len(cmpts) == 0:
      fatal_error("need at least one component model")
   if len(wts) != len(cmpts):
      fatal_error("number of wts isn't equal to the number of components")
   for f in cmpts:
      if not os.path.exists(f):
         fatal_error("file " + f + " doesn't exist")
   if not 1.0-0.001 < sum(wts) < 1.0+0.001:
      error("weights aren't normalized")
   if not args.force and os.path.exists(args.outlm):
      fatal_error("output LM <" + args.outlm + "> exists - move it or use -f")

   verbose("mixing " + str(len(cmpts)) + " components")

   batchno = 0
   n = get_order(cmpts[0])
   while len(cmpts) > 10:
      batchno += 1
      cmpts10 = cmpts[:10]
      sum10 = sum(wts[:10])
      wts10 = [w/sum10 for w in wts[:10]]
      f = tempfile.NamedTemporaryFile(delete=False, suffix=".gz")
      f.close()
      if train_mixlm(cmpts10, wts10, n, f.name):
         fatal_error("problem training models: " + join(cmpts10))
      verbose("mixed batch " + str(batchno) + " wt = " + str(sum10))
      cmpts[0:10] = [f.name]
      wts[0:10] = [sum10]
   if train_mixlm(cmpts, wts, n, args.outlm):
      fatal_error("problem training models: " + join(cmpts))
   verbose("wrote final mixture to " + args.outlm)
        
   # print(wts)
   # print(cmpts)

if __name__ == '__main__':
   main()
