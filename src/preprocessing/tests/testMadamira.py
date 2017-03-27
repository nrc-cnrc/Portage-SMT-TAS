# -*- coding: utf-8 -*-
# vim:expandtab:ts=3:sw=3

from __future__ import print_function
from __future__ import unicode_literals

import sys

try:
   import xmltodict
   from mock import patch, MagicMock, Mock, mock_open
except ImportError, e:
   print("Warning: Skipping unittest for madamira because xmltodict and/or mock is not installed.", file=sys.stderr)
else:
   import unittest
   import json
   
   from cStringIO import StringIO

   from madamira import TokenizedSentenceExtractor, RequestPackager

   # http://prmtl.net/post/mocking-stdout-in-tests/
   # http://schinckel.net/2013/04/15/capture-and-test-sys.stdout-sys.stderr-in-unittest.testcase/
   from contextlib import contextmanager
   @contextmanager
   def capture(command, *args, **kwargs):
      err, sys.stderr = sys.stderr, StringIO()
      try:
         command(*args, **kwargs)
         sys.stderr.seek(0)
         yield sys.stderr.read()
      finally:
         sys.stderr = err



   class TestTokenizedSentenceExtractor(unittest.TestCase):
      def test_markNonArbic_with_ascii(self):
         t = TokenizedSentenceExtractor()
         self.assertEqual(t._markNonArabic(u'#La_tour_Eiffel'), u'__ascii__#La_tour_Eiffel')
         self.assertEqual(t._markNonArabic(u'É'), u'É')
         self.assertEqual(t._markNonArabic(u'3423'), u'3423')


      def test_markNonArbic_with_Arabic(self):
         t = TokenizedSentenceExtractor()
         self.assertEqual(t._markNonArabic(u'ديسلر بسبب'), u'ديسلر بسبب')



   class TestRequestPackager(unittest.TestCase):
      def __init__(self, *args, **kwargs):
         super(TestRequestPackager, self).__init__(*args, **kwargs)

         self.sentences = ('First sentence', 'Last sentence')


      def testEscapeAscii(self):
         """
         Strings contains at least one [a-zA-Z] character should be prefixed with
         `__ascii__`.
         """
         r = RequestPackager(None)
         self.assertEqual(r._escapeAscii(u'hello'), u'__ascii__hello')
         self.assertEqual(r._escapeAscii(u'3213'), u'3213')
         self.assertEqual(r._escapeAscii(u'ق'), u'ق')


      def testEscapeHashtags(self):
         """
         We need to wrap Arabic hashtags in <hashtag> </hashtag>.
         """
         r = RequestPackager(None)
         t = r._escapeHashtags(u'#la_tour_Eiffel')
         self.assertTrue(isinstance(t, unicode))
         t = r._escapeHashtags(u'#ﺪﻴﺴﻟﺭ_ﺐﺴﺒﺑ')
         self.assertTrue(isinstance(t, unicode))

         # ASCII
         r = RequestPackager(None, markNonArabic=False, xmlishifyHashtags=False)
         self.assertEqual(r._escapeHashtags(u'#la_tour_Eiffel'), u'#la_tour_Eiffel')

         r = RequestPackager(None, markNonArabic=False, xmlishifyHashtags=True)
         self.assertEqual(r._escapeHashtags(u'#la_tour_Eiffel'), u'<hashtag> la tour Eiffel </hashtag>')

         r = RequestPackager(None, markNonArabic=True, xmlishifyHashtags=False)
         self.assertEqual(r._escapeHashtags(u'#la_tour_Eiffel'), u'__ascii__#la_tour_Eiffel')
         self.assertEqual(r._escapeHashtags(u'la#tour_Eiffel'), u'__ascii__la#tour_Eiffel')
         self.assertEqual(r._escapeHashtags(u'la tour Eiffel'), u'__ascii__la __ascii__tour __ascii__Eiffel')

         r = RequestPackager(None, markNonArabic=True, xmlishifyHashtags=True)
         self.assertEqual(r._escapeHashtags(u'#la_tour_Eiffel'), u'__ascii__#la_tour_Eiffel')

         # Arabic
         r = RequestPackager(None, markNonArabic=False, xmlishifyHashtags=False)
         self.assertEqual(r._escapeHashtags(u'#ديسلر_بسبب'), u'#ديسلر_بسبب')

         r = RequestPackager(None, markNonArabic=True, xmlishifyHashtags=True)
         self.assertEqual(r._escapeHashtags(u'#ديسلر_بسبب'), u'<hashtag> ديسلر بسبب </hashtag>')


      @patch('__builtin__.open', spec=open)
      def testEmpty(self, my_mock_open):
         """
         The user should receive a warning message if he doesn't provide any
         source sentences.  The request should got through but yield no in_seg.
         """
         source_sentence = MagicMock(name='source sentence', spec=open)
         source_sentence.return_value = iter([])
         my_mock_open.side_effect = source_sentence

         with patch('sys.stderr', new=StringIO()) as fake_stderr:
            r = RequestPackager(None)
            f = open('source')
            x = r(f)
            self.assertEqual(fake_stderr.getvalue(), 'Warning: There is no sentence in your input.\n')

         self.assertTrue("madamira_input" in x)
         madamira_input = x['madamira_input']

         self.assertTrue("in_doc" in madamira_input)
         in_doc = madamira_input['in_doc']
         self.assertEqual(in_doc['@id'], 'PortageLive')

         self.assertTrue("in_seg" in in_doc)
         in_seg = in_doc['in_seg']
         self.assertTrue(len(in_seg) == 0)


      def _verifyXMLWithTwoSentences(self, request):
         """
         Helper function that validates the xml request with two default source
         sentences.
         """
         self.assertTrue("madamira_input" in request)
         madamira_input = request['madamira_input']

         self.assertTrue("in_doc" in madamira_input)
         in_doc = madamira_input['in_doc']
         self.assertEqual(in_doc['@id'], 'PortageLive')

         self.assertTrue("in_seg" in in_doc)
         in_seg = in_doc['in_seg']
         self.assertTrue(len(in_seg) == 2)
         self.assertEqual(in_seg[0]['@id'], 'SENT0')
         self.assertEqual(in_seg[0]['#text'], 'First sentence')
         self.assertEqual(in_seg[1]['@id'], 'SENT1')
         self.assertEqual(in_seg[1]['#text'], 'Last sentence')


      @patch('__builtin__.open', spec=open)
      def testSimple(self, my_mock_open):
         """
         Simple use case where we package two source sentences and make sure they
         are properly encoded in the xml request.
         """
         def setupMock():
            source_sentence = MagicMock(name='source sentence file', spec=file)
            source_sentence.__enter__.return_value = source_sentence
            source_sentence.__iter__.return_value = iter(self.sentences)
            my_mock_open.return_value = source_sentence

         setupMock()

         request_packager = RequestPackager(None)
         with open('source') as source_sentence_file:
            request = request_packager(source_sentence_file)

         #print(json.dumps(request, indent=2))

         self._verifyXMLWithTwoSentences(request)


      @patch('__builtin__.open', spec=open)
      def testWithConfig(self, my_mock_open):
         """
         Verify that RequestPackager properly adds a madamira_configuration field.
         """
         def setupMock():
            source_sentence = MagicMock(name='source sentence file', spec=file)
            source_sentence.__enter__.return_value = source_sentence
            source_sentence.__iter__.return_value = iter(self.sentences)

            config = MagicMock(name='config file', spec=file)
            config.write.return_value = None
            config.__enter__.return_value = config
            config.read.return_value = '<?xml version="1.0" encoding="utf-8"?><madamira_configuration xmlns="urn:edu.columbia.ccls.madamira.configuration:0.1"></madamira_configuration>'

            my_mock_open.side_effect = (config, source_sentence)

         setupMock()

         request_packager = RequestPackager('config')
         with open('source') as source_sentence_file:
            request = request_packager(source_sentence_file)

         #print(json.dumps(request, indent=2))

         self._verifyXMLWithTwoSentences(request)

         self.assertTrue('madamira_configuration' in request['madamira_input'])
         madamira_configuration = request['madamira_input']['madamira_configuration']
         self.assertTrue('@xmlns' in madamira_configuration)
         self.assertEqual('urn:edu.columbia.ccls.madamira.configuration:0.1', madamira_configuration['@xmlns'])




if __name__ == '__main__':
   unittest.main()
