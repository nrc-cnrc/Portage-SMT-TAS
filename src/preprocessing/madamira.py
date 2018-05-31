#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file madamira.py
# @brief Wrapper for MADAMIRA with OSPL input/output
#
# @author Samuel Larking
#
# Note: requires xmltodict - run "pip install xmltodict" to install

# We encourage use of Python 3 features such as the print function.
from __future__ import print_function, unicode_literals, division, absolute_import

# curl --data @/opt/MADAMIRA-release-20160516-2.1/samples/xml/SampleInputFile.xml http://localhost:8223

import requests
import codecs
import sys
import os.path
from argparse import ArgumentParser
import xmltodict
import json
import re

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

   usage="madamira.py [options] [infile [outfile]]"
   help="""
   Takes OSPL input, wraps it in xml as required by Madamira's server, sends
   the request and extract the resulting tokenized sentences.

   Sample call:
   ridbom.sh < $MADAMIRA_HOME/samples/raw/SampleTextInput.txt | madamira.py --config $MADAMIRA_HOME/samples/sampleConfigFile.xml
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

   parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                       help="file encoding [%(default)s]")

   parser.add_argument("-c", '--config', dest="config", type=str, default="",
                       help="madamira config filename [%(default)s]")

   parser.add_argument("-u", '--url', dest="url", type=str, default="http://localhost:8223",
                       help="madamira server's url [%(default)s]")

   parser.add_argument("-s", '--scheme', dest="scheme", type=str, default="ATB",
                       help="scheme from config [%(default)s]")

   parser.add_argument("-f", '--form', dest="form", type=str, default="form0",
                       help="form from config [%(default)s]")

   # The following use the portage_utils version of open to open files.
   parser.add_argument("infile", nargs='?', type=open, default=sys.stdin,
                       help="input file [sys.stdin]")
   parser.add_argument("outfile", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output file [sys.stdout]")

   cmd_args = parser.parse_args()

   return cmd_args


"""
"""
def tokenize(xml_request, url='http://localhost:8223'):
   # The headers are used to send the request but also to prime MADAMIRA server.
   headers  = {'Content-Type': 'application/xml'} # set what your server accepts

   def startMADAMIRA():
      print('Starting a MADAMIRA server.', file=sys.stderr)
      import subprocess
      import os
      # How to start madamira
      # bash -c "exec -a MADAMIRA java ..." is a way of renaming a process to facilitate killing it later.
      # ( export PORTAGE=/opt/PortageII; bash -c "exec -a MADAMIRA java -Xmx5000m -Xms5000m -XX:NewRatio=3 -DPORTAGE_LOGS=$PORTAGE/logs  -jar $PORTAGE/bin/MADAMIRA-release-20160516-2.1/MADAMIRA-release-20160516-2.1.jar -s"; )
      portage       = os.environ.get('PORTAGE', '/opt/PortageII')
      madamira_home = os.environ.get('MADAMIRA_HOME', None)
      madamira_jar  = madamira_home + '/MADAMIRA-release-20160516-2.1.jar'
      if not os.path.exists(madamira_jar):
         fatal_error("Can't find madamira at specified location: " + madamira_jar)

      cmd = ['java', '-Xmx5000m', '-Xms5000m', '-XX:NewRatio=3', '-DPORTAGE_LOGS='+portage+'/logs', '-jar', 'MADAMIRA-release-20160516-2.1.jar', '-s']
      subprocess.Popen(cmd,
            stdout=open('/dev/null'),
            stderr=open('/dev/null'),
            cwd=madamira_home,  # MADAMIRA wants to be run from its installed directory.
            env={'PORTAGE': portage},  # Doesn't look like it's working for our command.
            preexec_fn=os.setpgrp)  # This will detach madamira from this process group.

   def waitMADAMIRA():
      from time import sleep
      # MADAMIRA takes about 11 seconds to load; longer on the GPSC.
      packager    = RequestPackager(None)
      xml_request = packager(['Priming.'])
      max_num_retries = 10
      for _ in xrange(max_num_retries):
         try:
            response = requests.post(url, data=xml_request, headers=headers, timeout=5)
            # TODO: Should we fatal_error if we can't start MADAMIRA server instead?
            if response.status_code == 200:
               break
         except requests.exceptions.ConnectionError:
            print('Waiting for MADAMIRA server...', file=sys.stderr)
            sleep(4)
         except:
            print('WHAT!? some unforseen event occured', file=sys.stderr)

   response = None
   try:
      response = requests.post(url, data=xml_request, headers=headers)
   except requests.exceptions.ConnectionError:
      # Can't connect to MADAMIRA server, may be it is not running, let's start it.
      startMADAMIRA()
      waitMADAMIRA()
      response = requests.post(url, data=xml_request, headers=headers)
   except Exception as e:
      fatal_error(str(e))
   finally:
      if response == None:
         fatal_error('Unable to connect to madamira server.')

      if response.status_code != 200:
         raise Exception('Something went wrong communicating with madamira server.')
      # The server does NOT return the encoding thus a quick hack is to set it
      # ourself and the rest of the code works fine.
      # https://github.com/kennethreitz/requests/issues/1604
      response.encoding = 'utf-8'

   return response


# The xml parser could have been untangle or BeautifulSoup but I chose
# xmltodict for its ability to parse and produce xml.
# https://github.com/stchris/untangle
# https://www.crummy.com/software/BeautifulSoup/
"""
Given a MADAMIRA response, TokenizedSentenceExtractor will produce one
sentence per line for the tokenized Arabic.
"""
class TokenizedSentenceExtractor():
   def __init__(self, scheme='ATB', form='form0', removeWaw=False, xmlishifyHashtags=False):
      self.scheme = scheme
      self.form = '@' + form
      self.removeWaw = removeWaw
      self.xmlishifyHashtags = xmlishifyHashtags
      self.re_latin_characters = re.compile(r'[a-zA-Z]')

   def _markNonArabic(self, token):
      """
      Token/word containing at least one latin letter is prefixed with `__ascii__`
      """
      if self.re_latin_characters.search(token) != None:
         token = '__ascii__' + token
      return token

   def _extractDocument(self, docs):
      """
      <out_doc id="ExampleDocument">
         <out_seg id="SENT1"> ... </out_seg>
         ...
         <out_seg id="SENTN"> ... </out_seg>
      </out_doc>
      """
      if isinstance(docs, dict):
         docs = [docs]
      for doc in docs:
         self._extractSegments(doc['out_seg'])

   def _extractSegments(self, segs):
      """
      <out_seg id="SENT1">
         ...
         <word_info>
            <word id="0" word="word1"> ... </word>
            ...
            <word id="n" word="wordn"> ... </word>
         </word_info>
      </out_seg>
      """
      if isinstance(segs, dict):
         segs = [segs]
      for seg in segs:
         word_info = seg['word_info']
         sentence = self._extractWords(word_info['word'])
         print(' '.join(sentence))

   def _extractWords(self, words):
      """
      <word id="0" word="word1">
         ...
         <tokenized scheme="ATB"> ... </tokenized>
         <tokenized scheme="MyD3"> ... </tokenized>
      </word>
      """
      if isinstance(words, dict):
         words = [words]
      tokens = []
      for word in words:
         tokenized = word['tokenized']
         if isinstance(tokenized, dict):
            tokenized = [tokenized]
         # Find the element with the desired scheme
         sc = next((t for t in tokenized if t['@scheme'] == self.scheme), None)
         tokens.extend(self._extractTokens(sc))
      if self.removeWaw and tokens[0] == u'و+':
         tokens = tokens[1:]
      if self.xmlishifyHashtags:
         for i, tok in enumerate(tokens):
            if tok not in ("<hashtag>", "</hashtag>"):
               tok = re.sub("&(?![a-zA-Z]+;)","&amp;",tok)
               tok = re.sub("<","&lt;",tok)
               tok = re.sub(">","&gt;",tok)
               tokens[i] = tok
      return tokens

   def _extractTokens(self, tokens):
      """
      <tokenized scheme="MyD3">
         <tok id="0" form0="tok0_form0"/>
         <tok id="1" form0="tok1_form0"/>
      </tokenized>
      """
      if tokens == None:
         fatal_error("We can find scheme={} in MADAMIRA's response".format(self.scheme))
      toks = tokens['tok']
      if isinstance(toks, dict):
         toks = [toks]
      return map(lambda t: t[self.form], toks)
      #return map(lambda t: self._markNonArabic(t[self.form]), toks)

   def __call__(self, response):
      """
      <madamira_output xmlns="urn:edu.columbia.ccls.madamira.configuration:0.1">
         <out_doc id="ExampleDocument">
            ...
         </out_doc>
      </madamira_output>
      """
      if response == None:
         fatal_error("There nothing to extract from madamira")
      madamira = xmltodict.parse(response.text)
      docs = madamira['madamira_output']['out_doc']
      try:
         self._extractDocument(docs)
      except TypeError as e:
         print(json.dumps(docs), file=sys.stderr)
         import traceback
         traceback.print_exc()
         raise e


"""
"""
class RequestPackager():
   def __init__(self, config_filename, markNonArabic=False, xmlishifyHashtags=False):
      """
      markNonArabic = True => prefix words containing at least one [a-zA-Z] with `__ascii`.
      xmlishifyHashtags = True => wraps hashtag that were not marked with `__ascii__` in <hashtag> your hashtag </hashtag>
      """
      self.config_filename = config_filename
      self.markNonArabic = markNonArabic
      self.xmlishifyHashtags = xmlishifyHashtags
      # According to Mada's rules to mark latin words with \@\@LAT
      self.re_latin_characters = re.compile(r'[a-zA-Z]')
      self.re_hashtag = re.compile(ur'(?<!__ascii__)#([^ #]+)')
      #self.re_hashtag = re.compile(ur'#([^ ]+)')
      #self.re_hashtag = re.compile(r'#([^ a-zA-Z]+)')

      self.config = None
      if self.config_filename != None and self.config_filename != '':
         with open(self.config_filename, 'r') as config_file:
            self.config = xmltodict.parse(config_file.read())

   def _escapeAscii(self, word):
      """
      If word contains at least on [a-zA-Z], prefix it with `__ascii__`.
      """
      if self.re_latin_characters.search(word) != None:
         word = u'__ascii__' + word
      return word

   def _escapeHashtags(self, sentence):
      """
      """
      if self.markNonArabic == False and self.xmlishifyHashtags == False:
         return sentence

      sentence = sentence.split()
      if self.markNonArabic:
         sentence = map(self._escapeAscii, sentence)
      if self.xmlishifyHashtags:
         sentence = map(lambda w: self.re_hashtag.sub(lambda x:
                    ' <hashtag> ' + re.sub('_', ' ', x.group(1)) + ' </hashtag> ', w), sentence)

      return ' '.join(sentence)

   def __call__(self, sentence_file):
      """
      Given a file containing one-sentence-per-line, assemble a valid xml
      request that can be send to MADAMIRA's server.
      """
      request = {
        'madamira_input' : {
           '@xmlns' : "urn:edu.columbia.ccls.madamira.configuration:0.1"
            }
        }

      if self.config != None:
         request['madamira_input'].update(self.config)
      #print(json.dumps(request, indent=2))

      in_seg = []
      for i, sentence in enumerate(sentence_file):
         sentence = sentence.strip()
         sentence = self._escapeHashtags(sentence)
         in_seg.append({'@id': 'SENT{}'.format(i), '#text': sentence})

      if len(in_seg) == 0:
         warn('There is no sentence in your input.')

      request['madamira_input']['in_doc'] = {
            '@id': 'PortageLive',
            'in_seg': in_seg
            }

      return request


def main():
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed

   cmd_args = get_args()

   # Handle file encodings:
   try:
      codecs.lookup(cmd_args.encoding)
   except LookupError:
      fatal_error("Illegal encoding specified for -enc (--encoding) option: '{0}'".format(cmd_args.encoding))

   sys.stdin  = codecs.getreader(cmd_args.encoding)(sys.stdin)
   sys.stdout = codecs.getwriter(cmd_args.encoding)(sys.stdout)
   cmd_args.infile = codecs.getreader(cmd_args.encoding)(cmd_args.infile)
   # The following allows stderr to handle non-ascii characters:
   sys.stderr = codecs.getwriter(cmd_args.encoding)(sys.stderr)

   packager = RequestPackager(cmd_args.config, cmd_args.markNonArabic, cmd_args.xmlishifyHashtags)
   request  = packager(cmd_args.infile)

   if cmd_args.debug:
      print('MADAMIRA request:\n', xmltodict.unparse(request, pretty=True), file=sys.stderr)

   xml_request = xmltodict.unparse(request).encode('utf-8-sig')
   response = tokenize(xml_request, cmd_args.url)

   if cmd_args.debug:
      print('MADAMIRA response:\n', response.text, file=sys.stderr)

   extractor = TokenizedSentenceExtractor(cmd_args.scheme, cmd_args.form, cmd_args.removeWaw, cmd_args.xmlishifyHashtags)
   extractor(response)


if __name__ == '__main__':
   main()
