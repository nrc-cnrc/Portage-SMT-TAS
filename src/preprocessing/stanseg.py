#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file stanseg.py
# @brief Wrapper for the Stanford Segmenter with our customized handling for hashtags and wa
#
# @author Eric Joanis

# We encourage use of Python 3 features such as the print function.
from __future__ import print_function, unicode_literals, division, absolute_import

import os
import sys
from argparse import ArgumentParser
from portage_utils import *

def get_args():
   """Command line argument processing."""

   usage="stanseg.py [options] [infile [outfile]]"
   help="""
   Takes OSPL input, pass it through the Stanford Segmenter with some
   additional conditional processing.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("-n", dest="markNonArabic", action='store_true', default=False,
                       help="Mark non Arabic words. [%(default)s]")

   parser.add_argument("-m", dest="xmlishifyHashtags", action='store_true', default=False,
                       help="xmlishify hashtags. [%(default)s]")

   parser.add_argument("-w", dest="removeWaw", action='store_true', default=False,
                       help="Remove beginning of sentence's w+. [%(default)s]")

   # The following use the portage_utils version of open to open files.
   parser.add_argument("infile", nargs='?', type=open, default=sys.stdin,
                       help="input file [sys.stdin]")
   parser.add_argument("outfile", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output file [sys.stdout]")

   cmd_args = parser.parse_args()

   return cmd_args


def main():
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed

   cmd_args = get_args()

   cmd_args.outfile.write(cmd_args.infile.read())


if __name__ == '__main__':
   main()
