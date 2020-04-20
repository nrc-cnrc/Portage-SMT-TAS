#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file prog.py
# @brief Briefly describe your script here.
#
# @author Write your name here
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2020, Sa Majeste la Reine du Chef du Canada /
# Copyright 2020, Her Majesty in Right of Canada

"""
This program shows how to:
   - turn on Python 3 features, including the print function
   - import portage_utils
   - use the argparse.ArgumentParser to handle various types of arguments
   - generate verbose or debug output
   - handle errors
   - open compressed files transparently
   - use codecs to handle different file encodings such as utf-8
   - call external programs
   - structure your code as a Python module
"""

# We encourage use of Python 3 features such as the print function.
from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import os.path
from argparse import ArgumentParser
import codecs
from subprocess import call, check_output, CalledProcessError

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

   usage="prog [options] mandatoryFile1 mandatoryFile2 [infile [outfile]]"
   help="""
   A brief description of what this program does. (Don't bother trying to format
   this text by including blank lines, etc: ArgumentParser is a control freak and
   will reformat to suit its tastes.)
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("-g", "--flag", dest="someFlag", action='store_true', default=False,
                       help="some flag option [%(default)s]")

   parser.add_argument("-s", dest="str_opt", type=str, default="",
                       help="some string option [%(default)s]")
   parser.add_argument("-i", dest="int_opt", type=int, default=1,
                       help="some integer option [%(default)s]")
   parser.add_argument("-f", dest="float_opt", type=float, default=1.0,
                       help="some float option [%(default)s]")
   parser.add_argument("-l", dest="list_opt", nargs="*", type=str, default=[],
                       help="list of string operands [%(default)s]")
   parser.add_argument("-c", dest="choice_opt", nargs="?",
                       choices=("choice1","choice2"), const="choice1", default=None,
                       help="one of choice1, choice2; -c alone implies %(const)s "
                            "[%(default)s]")
   parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                       help="file encoding [%(default)s]")

   parser.add_argument("mandatoryFile1", type=open, help="file 1")
   parser.add_argument("mandatoryFile2", type=open, help="file 2")

   # The following use the portage_utils version of open to open files.
   parser.add_argument("infile", nargs='?', type=open, default=sys.stdin,
                       help="input file [sys.stdin]")
   parser.add_argument("outfile", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output file [sys.stdout]")

   cmd_args = parser.parse_args()

   # info, verbose, debug all print to stderr.
   info("arguments are:")
   for arg in cmd_args.__dict__:
      info("  {0} = {1}".format(arg, getattr(cmd_args, arg)))
   verbose("verbose flag is set.")    # printed only if verbose flag is set by -v.
   debug("debug flag is set.")        # printed only if debug flag is set by -d.
   return cmd_args

def main():
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed

   cmd_args = get_args()

   # Handle file encodings:
   try:
      codecs.lookup(cmd_args.encoding)
   except LookupError:
      fatal_error("Illegal encoding specified for -enc (--encoding) option: '{0}'".format(cmd_args.encoding))

   infile = codecs.getreader(cmd_args.encoding)(cmd_args.infile)
   outfile = codecs.getwriter(cmd_args.encoding)(cmd_args.outfile)
   # The following allows stderr to handle non-ascii characters:
   sys.stderr = codecs.getwriter(cmd_args.encoding)(sys.stderr)

   # Call an external program without invoking a shell and checking the return code:
   verbose("Calling 'ls xxxx' not using a shell.")
   ret_code = call(["ls", "xxxx"])
   if ret_code is not 0:
      warn("program xxxx not found.")

   # Call an external program invoking a shell and checking the return code:
   verbose("Calling 'ls | wc -l' using a shell.")
   ret_code = call("ls | wc -l", shell=True)
   if ret_code is not 0:
      error("'ls | wc -l' failed, returned:", ret_code)

   # Call an external program not invoking a shell and checking the output:
   verbose("Calling 'ls .' checking the output.")
   try:
      contents = check_output(["ls", "."])
   except CalledProcessError as err:
      fatal_error("'ls .' failed, returned:", err.returncode)
   info("ls returned:", contents.replace('\n',' '))

   # Copy the infile to outfile
   verbose("Copying infile to outfile.")
   for line in infile:
      print(line, file=outfile, end='')


if __name__ == '__main__':
   main()
