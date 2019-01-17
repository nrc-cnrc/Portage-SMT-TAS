#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file stanseg-pre.py
# @brief Pre-processing before calling the Stanford Segmenter in stanseg.py
#
# @author Eric Joanis

from __future__ import print_function, unicode_literals, division, absolute_import

import os
import sys
import codecs
import re
from argparse import ArgumentParser
from portage_utils import *

def get_args():
   """Command line argument processing."""

   usage="stanseg-pre.py [options] [infile [outfile]]"
   help="""
   Do the required pro-processing before calling the Stanford Segmenter
   in stanseg.py. Not intended to be called directly.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("-m", dest="xmlishifyHashtags", action='store_true', default=False,
                       help="xmlishify hashtags. [%(default)s]")

   # The following use the portage_utils version of open to open files.
   parser.add_argument("infile", nargs='?', type=open, default=sys.stdin,
                       help="input file [sys.stdin]")
   parser.add_argument("outfile", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output file [sys.stdout]")

   cmd_args = parser.parse_args()

   return cmd_args

class RequestPackager():
   def __init__(self, xmlishifyHashtags=False):
      self.xmlishifyHashtags = xmlishifyHashtags
      self.re_hashtag = re.compile(ur'(?<!__ascii__)#([^ #]+)')

   def _escapeHashtags(self, sentence):
      """
      """
      if self.xmlishifyHashtags == False:
         return sentence

      sentence = sentence.split()
      if self.xmlishifyHashtags:
         sentence = map(lambda w: self.re_hashtag.sub(lambda x:
                    ' <hashtag> ' + re.sub('_', ' ', x.group(1)) + ' </hashtag> ', w), sentence)

      return ' '.join(sentence)

   def __call__(self, sentence):
      return self._escapeHashtags(sentence)

def main():
   cmd_args = get_args()

   infile = codecs.getreader('utf-8')(cmd_args.infile)
   outfile = codecs.getwriter('utf-8')(cmd_args.outfile)
   r = RequestPackager(xmlishifyHashtags=cmd_args.xmlishifyHashtags)
   for line in infile:
      print(r(line.rstrip()), file=outfile)


if __name__ == '__main__':
   main()
