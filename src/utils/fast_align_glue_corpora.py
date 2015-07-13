#!/usr/bin/env python
# @file fast_align_glue_corpora.py
# @brief Preprocessing for fast_align => s ||| t.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada


# We encourage use of Python 3 features such as the print function.
from __future__ import print_function

from itertools import izip
import sys
import os.path
from argparse import ArgumentParser

# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path)
# to arrange that portage_utils will be imported from src/utils.
if sys.argv[0] not in ('', '-c'):
   bin_path = os.path.dirname(sys.argv[0])
   if os.path.basename(bin_path) != "bin":
      sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

# portage_utils provides a bunch of useful and handy functions, including:
#   HelpAction, VerboseAction, DebugAction (helpers for argument processing)
#   printCopyright
#   info, verbose, debug, warn, error, fatal_error
#   open (transparently open stdin, stdout, plain text files, compressed files or pipes)
from portage_utils import *

def get_args():
   """Command line argument processing."""

   usage="fast_align_glue_corpora.py source target [outfile]"
   help="""
   Given source and target, create for each sentence pair:
   source ||| target
   which is the input file format for fast_align.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("source", type=open, help="source corpora file")
   parser.add_argument("target", type=open, help="target corpora file")

   # The following use the portage_utils version of open to open files.
   parser.add_argument("outfile", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output file [sys.stdout]")

   return parser.parse_args()


def main():
   printCopyright("fast_align_glue_corpora.py", 2015);
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed

   cmd_args = get_args()

   for x, y in izip(cmd_args.source, cmd_args.target):
      x = x.strip()
      y = y.strip()
      print("{0} ||| {1}".format(x, y), file=cmd_args.outfile)


if __name__ == '__main__':
   main()
