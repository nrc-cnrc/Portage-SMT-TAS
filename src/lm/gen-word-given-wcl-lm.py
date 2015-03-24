#!/usr/bin/env python

# @file gen-word-given-wcl-lm.py
# @brief Generate a 1-gram LM (in ARPA format) providing log probabilities
# p(w | wcl(w)), where wcl(w) is the word class corresponding to word w
# (e.g. trained using mkcls).
# 
# @author Darlene Stewart
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import codecs
import os.path
from argparse import ArgumentParser, FileType
from subprocess import check_output, CalledProcessError
import math

# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path)
# to arrange that portage_utils will be imported from src/utils.
if sys.argv[0] not in ('', '-c'):
   bin_path = os.path.dirname(sys.argv[0])
   if os.path.basename(bin_path) != "bin":
      sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

from portage_utils import *


def get_args():
   """Command line argument processing."""
   
   usage="gen-word-given-wcl-lm.py [options] classes_file corp1 [corp2 ...]"
   help="""
   Generate a 1-gram LM from 1 or more corpora files providing log probabilites
   p(w | wcl(w)), where wcl(w) is the word class corresponding to word w,
   writing the ARPA format LM output to stdout.
   """
   
   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("classes_file", type=FileType('r'),
                       help=".classes file (e.g. from mkcls)")

   parser.add_argument("corp_files", nargs="+", type=FileType('r'),
                       help="corpora files to train the LM")

   cmd_args = parser.parse_args()
   
   return cmd_args

def main():
   printCopyright("gen-word-given-wcl-lm.py", 2013);
   
   cmd_args = get_args()
   
   sys.stderr = codecs.getwriter("utf-8")(sys.stderr)  # allow utf-8 in error msgs
   sys.stdout = codecs.getwriter("utf-8")(sys.stdout)  # allow utf-8 output

   # Note: We read the whole file and split on just '\n' to be more resilient
   #       to words containing control characters such as GS (0x1d).

   # Read the classes file.
   classes_file = codecs.getreader("utf-8")(cmd_args.classes_file)
   classes_file_lines = classes_file.read().split("\n")
   debug ("classes file length:", len(classes_file_lines))
   cls = {}
   line_no = 0
   for line in classes_file_lines:
      line_no += 1
      if len(line) == 0 and line_no == len(classes_file_lines): break
      debug(line_no, ": input length:", len(line), "content:", line, 
            "(", ":".join("{0:x}".format(ord(c)) for c in line), ")")
      toks = line.split("\t")
      if len(toks) != 2:
         fatal_error("Error in class file", classes_file.name, "line", line_no,
                     "expected exactly one tab character, found", len(toks)-1,
                     "in line:", line, 
                     "(", ":".join("{0:x}".format(ord(c)) for c in line), ")",
                     "length:", len(line))
      cls[toks[0]] = int(toks[1])
   del classes_file_lines

   # Map corpora words to word classes and determine word class frequency counts.
   cmd = "zcat -f {0} | word2class - {1} | get_voc -c".format(
            " ".join(c.name for c in cmd_args.corp_files), classes_file.name)
   try:
      class_freq_lines = unicode(check_output(cmd, shell=True), "utf-8").split("\n")
   except CalledProcessError as err:
      fatal_error("'{0}' failed, returned:".format(cmd), err.returncode)
   if len(class_freq_lines[-1]) == 0:  # remove empty line due to '\n' at string end
      del class_freq_lines[-1]
   class_freq = {}
   line_no = 0
   try:
      for line in class_freq_lines:
         line_no += 1
         debug("class freq: input length:", len(line), "content:", line, 
            "(", ":".join("{0:x}".format(ord(c)) for c in line), ")")
         toks = line.split()
         if len(toks) != 2:
            fatal_error("Error in get_voc output, expected 2 tokens but found", 
                        len(toks), "in line:", line)
         class_freq[int(toks[0])] = int(toks[1])
   except CalledProcessError as err:
      fatal_error("'{0}' failed, returned:".format(cmd), err.returncode)
   del class_freq_lines
   
   # Determine determine word class frequency counts the the corpora.
   cmd = "zcat -f {0} | get_voc -c".format(
            " ".join(c.name for c in cmd_args.corp_files), classes_file.name)
   try:
      word_freq_lines = unicode(check_output(cmd, shell=True), "utf-8").split("\n")
   except CalledProcessError as err:
      fatal_error("'{0}' failed, returned:".format(cmd), err.returncode)
   if len(word_freq_lines[-1]) == 0:  # remove empty line due to '\n' at string end
      del word_freq_lines[-1]
   word_freq = {}
   line_no = 0
   for line in word_freq_lines:
      line_no += 1
      debug("word freq: input length:", len(line), "content:", line, 
            "(", ":".join("{0:x}".format(ord(c)) for c in line), ")")
      toks = line.split(" ")
      if len(toks) != 2:
         fatal_error("Error in get_voc output, expected 2 tokens but found", 
                     len(toks), "in line:", line)
      word_freq[toks[0]] = int(toks[1])
   del word_freq_lines
   
   # Compute the log probabilities and write the ARPA format LM file.
   print("\\data\\")
   print("ngram 1={0}".format(len(word_freq)))
   print("")
   print("\\1-grams:")
   for w in sorted(word_freq.keys()):
      prob = math.log10(float(word_freq[w]) / class_freq[cls[w]])
      print("{0:.7f}\t{1}".format(prob, w))
   print("")
   print("\\end\\")

if __name__ == '__main__':
   main()
