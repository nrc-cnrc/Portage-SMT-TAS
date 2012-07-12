#!/usr/bin/env python
# $Id$

# @file lm-filter.py
# @brief Filters out lm entries based on a minimum ngram counts.
# 
# @author Samuel Larkin
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

# We encourage use of Python 3 features such as the print function.
from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import os.path
from argparse import ArgumentParser
import codecs
from subprocess import call, check_output, CalledProcessError
from tempfile import NamedTemporaryFile

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
   
   usage="lm-filter.py [options] languageModel counts thresholds"
   help="""
   Filter a Language Model based on some Ngram count threshold.
   The languageModel and count files must be sorted in the same order.  This
   can esaily be achieved by invoking estimate-ngram -write-lm lm.gz
   -write-count counts.gz.

   example:
      lm-filter.py lm.gz counts.gz
   """
   
   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                       help="file encoding [%(default)s]")

   parser.add_argument("languageModel", type=open, help="ARPA Language Model")
   parser.add_argument("counts", type=open, help="ngram count file.")
   parser.add_argument("threshold", nargs="*", type=str, default=[],
                       help="ngram count file.")

   # The following use the portage_utils version of open to open files.
   parser.add_argument("-o", dest="outfile", type=lambda f: open(f,'w'), 
                       default=sys.stdout, 
                       help="output file for filtered language model [stdout]")

   cmd_args = parser.parse_args()
   
   # info, verbose, debug all print to stderr.
   info("arguments are:")
   for arg in cmd_args.__dict__:
      info("  {0} = {1}".format(arg, getattr(cmd_args, arg)))
   verbose("verbose flag is set.")    # printed only if verbose flag is set by -v.
   debug("debug flag is set.")        # printed only if debug flag is set by -d.
   return cmd_args

def main():
   def write_entry():
      """ Helper function that make sure counts are kept in sync when printing an entry."""
      ngram_counts[order] += 1
      print(lm, file=temp_file)

   def read_count():
      """ Reads a count entry from the count file and validates the format."""
      global count_file_line_number
      count = cmd_args.counts.readline().strip('\n').split('\t')
      count_file_line_number += 1
      if len(count) != 2:
         fatal_error("Invalid format of counts in {}: {}".format(cmd_args.counts, count))
      return count

   printCopyright("lm-filter.py", 2012);
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed
   
   cmd_args = get_args()

   # Validate threshold values.
   def validateThresholds():
      previous = 0
      for t in cmd_args.threshold:
         if t < previous:
            fatal_error("Thresholds must be ascending in value", cmd_args.threshold)
         previous = t
   validateThresholds()

   # Handle file encodings:
   try:
      codecs.lookup(cmd_args.encoding)
   except LookupError:
      fatal_error("Illegal encoding specified for -enc (--encoding) option: '{0}'".format(cmd_args.encoding))
   
   cmd_args.languageModel = codecs.getreader(cmd_args.encoding)(cmd_args.languageModel)
   cmd_args.counts        = codecs.getreader(cmd_args.encoding)(cmd_args.counts)
   temp_file              = codecs.getwriter(cmd_args.encoding)(NamedTemporaryFile( prefix='lm-filter.', delete=False, mode='w' ))
   # The following allows stderr to handle non-ascii characters:
   sys.stderr = codecs.getwriter(cmd_args.encoding)(sys.stderr)
   
   order = -1
   ngram_counts = []
   dropped_ngram_counts = []
   threshold = 0
   for lm in cmd_args.languageModel:
      lm = lm.strip('\n')

      # Skip empty lines
      if lm == "": continue

      # N-gram section header
      if lm.endswith("-grams:"):
         order += 1
         if order < len(cmd_args.threshold):
            if threshold > cmd_args.threshold[order]:
               fatal_error("Your threshold must be ascending.")
            threshold = int(cmd_args.threshold[order])
         ngram_counts.append(0)
         dropped_ngram_counts.append(0)
         info('Using a minimum of {c} counts to prune {o}-gram'.format(c=threshold, o=order+1))
         print("\n", lm, sep='', file=temp_file)
         continue

      parts = lm.split('\t');
      if len(parts) == 1:
         continue
      elif 1 < len(parts) <= 3:
         ngram = parts[1]
         if ngram == '</s>':
            write_entry()
            continue
         count = read_count()
         while ngram != count[0]:
            debug("Reading next count: ", ngram, "<>", count[0])
            count = read_count()
         # Same ngram with a count higher then the threshold.
         if ngram == count[0] and int(count[1]) >= threshold:
            write_entry()
         else:
            dropped_ngram_counts[order] += 1
            debug("Dropping: ", threshold, lm, count)
      else:
         fatal_error("Error parsing the language model file ({}).".format(lm))
   
   # Start writing the Language Model's header.
   output = codecs.getwriter(cmd_args.encoding)(cmd_args.outfile)
   print("\n\\data\\", file=output)
   for o, c in enumerate(ngram_counts):
      print('ngram {o}={c}'.format(o=o+1, c=c), file=output)

   # Write out the filtered N-gram sections.
   temp_file.close()
   debug("Temp stream is ", temp_file.name)
   temp_file = codecs.getreader(cmd_args.encoding)(open(temp_file.name, 'r'))
   for l in temp_file:
      print(l, end='', file=output)
   print("\n\\end\\", file=output)

   # Do some clean up by removing the temp file.
   os.remove(temp_file.name)

   for o, c in enumerate(dropped_ngram_counts):
      print('{c} {o}-ngram were dropped!'.format(o=o+1, c=c), file=output)


count_file_line_number = 0
if __name__ == '__main__':
   try:
      main()
   except UnicodeDecodeError as err:
      fatal_error(err, count_file_line_number, count)

