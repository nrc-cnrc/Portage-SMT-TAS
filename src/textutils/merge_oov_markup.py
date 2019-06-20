#!/usr/bin/env python

# @file merge_oov_markup.py
# @brief Merge canoe-style markup from two streams.
#
# @author Darlene A. Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

"""
This script merges canoe-style markup from two input streams: the original
XML marked up source input file and the XMLish OOV markup output from
'canoe -oov write-src-marked' for the same source input.

This script is needed when we want to use canoe to mark OOVs for us to further
preprocess them before calling canoe to translate the input. In such a case,
we need to merge the OOV markup back into the original source stream.
"""

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
from argparse import ArgumentParser, FileType
import codecs
import re

# Make sure the location of portage_utils is in your PYTHONPATH.
from portage_utils import *

def get_args():
   """Command line argument processing."""

   usage="merge_oov_markup.py [options] markup-file oov-file [out-file]"
   help="""
   Merge canoe-style markup from two input streams: the original XML tag marked
   up source input file and the XMLish OOV markup output from 'canoe -oov
   write-src-marked'.

   The two input files represent the same source input, except the first file
   contains any regular source markup except OOV markup as well as XML
   escapes (&gt; &lt; &amp;), while only the OOVs are marked in the second file.

   XML elements may be nested, and may contain OOVs. Proper XML escapes
   are assumed to be used within XML elements.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("markup_file", metavar="markup-file", type=open,
                       help="XMLish marked up source input file")
   parser.add_argument("oov_file", metavar="oov-file", type=open,
                       help="XMLish OOV marked up source input file")
   parser.add_argument("out_file", metavar="out-file", nargs='?',
                       type=lambda f: open(f,'w'), default=sys.stdout,
                       help="output file [sys.stdout]")

   cmd_args = parser.parse_args()

   return cmd_args

def main():
   cmd_args = get_args()

   # Allow the file streams to handle non-ascii characters.
   markup_file = codecs.getreader("utf-8")(cmd_args.markup_file)
   oov_file = codecs.getreader("utf-8")(cmd_args.oov_file)
   out_file = codecs.getwriter("utf-8")(cmd_args.out_file)
   sys.stderr = codecs.getwriter("utf-8")(sys.stderr)

   line_number = 0;

   # tok_re matches an XMLish element (<tag ...> or </tag>) or Portage token
   tok_re = re.compile(r'((?:<([^ \t>]+)(?:"[^"]*"|[^">])*>)|(?:[^ \t\n]+))')


   for markup_line in markup_file:
      line_number += 1
      markup_items = tok_re.findall(markup_line)

      oov_line = oov_file.readline()
      if not oov_line:
         fatal_error("EOF encountered in oov-file before EOF in markup-file.")
      oov_toks = split(oov_line)

      out_line = []

      tok_index = 0
      # markup_items is a list of 2-tuples representing XML tags or plain tokens.
      # tuple[0] holds the text of the XMLish element or plain token.
      # tuple[1] holds the tag name for an XMLish element or '' for a plain token.
      for text, tag in markup_items:
         if tag:
            out_line.append(text)
         else:
            oov_text = oov_toks[tok_index]
            if oov_text.startswith("<oov>") and oov_text.endswith("</oov>"):
               out_line.append("<oov>{0}</oov>".format(text))
            else:
               out_line.append(text)
            tok_index += 1

      print(*out_line, file=out_file)

   oov_line = oov_file.readline()
   if oov_line:
      fatal_error("EOF encountered in markup-file before EOF in oov-file.", line_number)


if __name__ == '__main__':
   main()
