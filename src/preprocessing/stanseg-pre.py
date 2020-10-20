#!/usr/bin/env python2
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
   def __init__(self, xmlishifyHashtags=False, escapeHandles=False):
      self.xmlishifyHashtags = xmlishifyHashtags
      self.escapeHandles = escapeHandles
      self.re_hashtag = re.compile(ur'(?<!__ascii__)#([^ #]+)')
      self.re_handle = re.compile(ur'^(@[A-Za-z0-9_]+)(.*)')
      self.re_handle_dict = {
         '@': 'ZA', '_': 'ZU', 'Z': 'ZZ',
         '0': 'Za', '1': 'Zb', '2': 'Zc', '3': 'Zd', '4': 'Ze',
         '5': 'Zf', '6': 'Zg', '7': 'Zh', '8': 'Zi', '9': 'Zj',
      }


   def _escapeHashtags(self, sentence):
      sentence = sentence.split()
      sentence = map(lambda w: self.re_hashtag.sub(lambda x:
                 ' <hashtag> ' + re.sub('_', ' ', x.group(1)) + ' </hashtag> ', w), sentence)
      return ' '.join(sentence)

   def _escapeTwitterHandles(self, sentence):
      tokens = sentence.split()
      for i, tok in enumerate(tokens):
         match = self.re_handle.match(tok)
         if match:
            escaped_tok="TWITTERHANDLE"
            for c in match.group(1):
               escaped_tok += self.re_handle_dict.get(c, c)
            #print('handle {} -> {}'.format(tok, escaped_tok), file=sys.stderr)
            tokens[i] = escaped_tok + ' ' + match.group(2)
      return ' '.join(tokens)

   def __call__(self, sentence):
      if self.escapeHandles:
         sentence = self._escapeTwitterHandles(sentence)
      if self.xmlishifyHashtags:
         sentence = self._escapeHashtags(sentence)
      return sentence

def main():
   cmd_args = get_args()

   infile = codecs.getreader('utf-8')(cmd_args.infile)
   outfile = codecs.getwriter('utf-8')(cmd_args.outfile)
   r = RequestPackager(xmlishifyHashtags=cmd_args.xmlishifyHashtags, escapeHandles=True)
   for line in infile:
      print(r(line.rstrip()), file=outfile)


if __name__ == '__main__':
   main()
