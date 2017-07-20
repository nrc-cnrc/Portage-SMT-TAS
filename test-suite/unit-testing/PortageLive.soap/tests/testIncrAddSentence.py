#!/usr/bin/env python
# vim:expandtab:ts=3:sw=3

#import zeep
#client = zeep.Client(wsdl=url)

from suds.cache import DocumentCache
from suds.client import Client
from suds import WebFault
import unittest
import logging
import requests
import time
import os



logging.basicConfig(level=logging.CRITICAL)
# If you need to debug what is happening, uncomment the following line
#logging.basicConfig(level=logging.DEBUG)

url = 'http://127.0.0.1'

class TestIncrAddSentence(unittest.TestCase):
   """
   Using PortageLiveAPI's WSDL deployed on a web server, we test SOAP calls to
   incrAddSentence().
   """


   def __init__(self, *args, **kwargs):
      super(TestIncrAddSentence, self).__init__(*args, **kwargs)

      DocumentCache().clear()
      self.url = url + ':' + os.getenv('PHP_PORT', 8756)
      self.WSDL = self.url + '/PortageLiveAPI.wsdl'
      self.client = Client(self.WSDL)
      # Kludge to trigger a server side unittest.
      # The document_level_model_id must match PortageLiveLib::$MAGIC_UNITTEST_DOCUMENT_ID
      self.context = 'unittest.rev.en-fr'
      self.document_level_model_id = 'PORTAGE_UNITTEST_4da35'
      self.source_sentence = "'home'"
      self.target_sentence = '"maison"'


   def test_no_argument(self):
      """
      incrAddSentence() should warn the user that it needs
      some parameters.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence()
      self.assertEqual(cm.exception.message, u"Server raised fault: 'Missing parameter'")


   def test_all_arguments_null(self):
      """
      incrAddSentence() expects 3 arguments that cannot be
      None/NULL.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(None, None, None, None, None)
      self.assertEqual(cm.exception.message, u"Server raised fault: 'Missing parameter'")


   def test_no_document_level_model_id(self):
      """
      It is invalid to use the empty string as document level model ID.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context, '', '', '')
      self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a valid document_level_model_ID.'")


   def test_no_source_sentence(self):
      """
      The source sentence cannot be empty.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context, self.document_level_model_id, '', '')
      self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a source sentence.'")


   def test_no_target_sentence(self):
      """
      The target sentence cannot be empty.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context, self.document_level_model_id, self.source_sentence, '')
      self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a target sentence.'")


   @unittest.skip("Should we check for too many parameters?")
   def test_to_many_parameters(self):
      """
      TODO: Should we get some sort of message if we provide an invalid number
      of arguments
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence( \
               self.context, \
               self.document_level_model_id, \
               self.source_sentence, \
               self.target_sentence,
               'extra_dummy_argument')
      self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a target sentence.'")


   def test_basic_valid_usage(self):
      """
      This tests a valid call to incrAddSentence() where
      document_level_model_id is valid, source sentence is valid and target
      sentence is also valid.
      - The SOAP call should return true since it's supposed to be able to add
        this sentence pair to the queue.
      - The training phase should have inserted the sentence pair in the
        corpora.
      """
      import random
      UID = str(random.randint(0, 100000))
      source = self.source_sentence + str(time.time()) + UID
      target = self.target_sentence + str(time.time()) + UID
      result = self.client.service.incrAddSentence(self.context, self.document_level_model_id, source, target)
      self.assertEqual(result, True, \
            msg='SOAP call failed to add a sentence pair')
      # TODO:
      # - Fetch the corpora and make sure it contains our sentence pair

      r = requests.get(self.url + '/plive/DOCUMENT_LEVEL_MODEL_' + self.document_level_model_id + '/corpora')
      self.assertEqual(r.status_code, 200, \
            msg='Failed fetching the corpora file for '.format(self.document_level_model_id))

      reference_sentence_pair = source + '\t' + target
      reference_in_corpora = filter(lambda sp: sp.find(reference_sentence_pair) != -1, r.text.split('\n'))
      self.assertEqual(len(reference_in_corpora), 1, \
            msg='There should be exactly one occurence of our sentence pair in the corpora.')



if __name__ == '__main__':
   unittest.main()

