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
import codecs
import re
from argparse import ArgumentParser
from portage_utils import *
from subprocess import *

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

class RequestPostprocessor():
   def __init__(self, removeWaw=False, xmlishifyHashtags=False, unescapeHandles=False):
      self.removeWaw = removeWaw
      self.xmlishifyHashtags = xmlishifyHashtags
      self.unescapeHandles = unescapeHandles
      self.re_escaped_handle = re.compile(ur'^TWITTERHANDLEZA[a-zA-Z]+$')
      self.re_handle_reverse_dict = {
         'A': '@', 'U': '_', 'Z': 'Z',
         'a': '0', 'b': '1', 'c': '2', 'd': '3', 'e': '4',
         'f': '5', 'g': '6', 'h': '7', 'i': '8', 'j': '9',
      }

   def _escapeXMLChars(self, sentence):
      # < hashtag > -> <hashtag> and </ hashtag > -> </hashtag> since Stan Seg toks them
      sentence = re.sub("(^| )<(/|) hashtag >($| )", "\g<1><\g<2>hashtag>\g<3>", sentence)
      # We need to run this RE sub twice because the first misses a < hashtag >
      # immediately after a </ hashtag >
      sentence = re.sub("(^| )<(/|) hashtag >($| )", "\g<1><\g<2>hashtag>\g<3>", sentence)

      tokens = sentence.split()
      for i, tok in enumerate(tokens):
         if tok not in ("<hashtag>", "</hashtag>"):
            tok = re.sub("&(?![a-zA-Z]+;)","&amp;",tok)
            tok = re.sub("<","&lt;",tok)
            tok = re.sub(">","&gt;",tok)
            tokens[i] = tok
      return ' '.join(tokens)

   def _unescapeTwitterHandle(self, sentence):
      tokens = sentence.split()
      prefix_len = len("TWITTERHANDLE")
      for i, tok in enumerate(tokens):
         if self.re_escaped_handle.match(tok):
            #print("unescape {}".format(tok), file=sys.stderr)
            tokens[i] = re.sub('Z(.)',
               lambda m: self.re_handle_reverse_dict.get(m.group(1), m.group(1)),
               tok[prefix_len:])
      return ' '.join(tokens)

   def _removeWaw(self, sentence):
      return re.sub(u'^و\+ ', '', sentence)

   def __call__(self, sentence):
      if self.xmlishifyHashtags:
         sentence = self._escapeXMLChars(sentence)
      if self.removeWaw:
         sentence = self._removeWaw(sentence)
      if self.unescapeHandles:
         sentence = self._unescapeTwitterHandle(sentence)
      return sentence

def run_normalize(infile):
   """
   Pass the input through normalize-unicode.pl ar since the Stanford Segmenter
   does not recognize characters in their presentation form, only in their
   canonical form.
   """
   norm_cmd = "normalize-unicode.pl ar"
   p = Popen(norm_cmd, shell=True, stdin=infile, stdout=PIPE).stdout
   return p

def run_prepro(infile, xmlishifyHashtags):
   """
   Run the preprocessing in a separate script before calling the stanford
   segmenter itself. The code might have been placed in a child process in this
   script by using os.fork(), but writing a separate script is simpler (and
   lazier, I know...)
   """
   pre_cmd = "stanseg-pre.py"
   if xmlishifyHashtags:
      pre_cmd += " -m"
   p = Popen(pre_cmd, shell=True, stdin=infile, stdout=PIPE).stdout
   return p

def run_stan_seg(infile):
   """Run the Stanford Segmenter itself"""
   stanseg_home = os.environ.get('STANFORD_SEGMENTER_HOME', None)
   stanseg_classifier = "arabic-segmenter-atb+bn+arztrain.ser.gz"
   stanseg_cmd = "java -mx4g " + \
      "edu.stanford.nlp.international.arabic.process.ArabicSegmenter " + \
      "-loadClassifier " + stanseg_home + "/data/" + stanseg_classifier + \
      " -prefixMarker + -suffixMarker + -domain arz -nthreads 1"
   p = Popen(stanseg_cmd, shell=True, stdin=infile, stdout=PIPE).stdout
   #print(p)
   #print(p[0])
   return p

def main():
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed

   print("DEPRECATED WARNING: stanseg.py was the first attempt to wrap the Stanford Segmenter and is somewhat buggy. It's here for development and documentation purposes only. Use stanseg.pl instead.\n", file=sys.stderr)

   cmd_args = get_args()

   pipe1 = run_normalize(cmd_args.infile)
   pipe2 = run_prepro(pipe1, cmd_args.xmlishifyHashtags)
   pipe3 = codecs.getreader('utf-8')(run_stan_seg(pipe2))

   outfile = codecs.getwriter('utf-8')(cmd_args.outfile)

   post = RequestPostprocessor(cmd_args.removeWaw, cmd_args.xmlishifyHashtags,
      unescapeHandles=True)
   for line in pipe3:
      print(post(line.rstrip()), file=outfile)


if __name__ == '__main__':
   main()
