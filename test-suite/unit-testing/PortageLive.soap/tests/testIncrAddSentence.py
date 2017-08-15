#!/usr/bin/env python
# vim:expandtab:ts=3:sw=3

# @file testIncrStatus.py
# @brief Test SOAP calls to incrAddSentence using a deployed PortageLive web server.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

#import zeep
#client = zeep.Client(wsdl=url)

from suds.cache import DocumentCache
from suds.client import Client
from suds import WebFault
import unittest
import logging
import requests
import time
import random
import os
import sys

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
      self.context = 'unittest.rev.en-fr'
      self.document_model_id = 'PORTAGE_UNITTEST_4da35'
      self.source_sentence = "'home'"
      self.target_sentence = '"maison"'

      self.document_model_dir = os.path.join("doc_root", "plive",
                                   "DOCUMENT_MODEL_"+self.document_model_id)
      if (os.path.isdir(self.document_model_dir)):
         shutil.rmtree(self.document_model_dir)

   def test_01_no_argument(self):
      """
      incrAddSentence() should warn the user that it needs some parameters.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence()
      self.assertEqual(cm.exception.message, "Server raised fault: 'Missing parameter'")

   def test_02_all_arguments_null(self):
      """
      incrAddSentence() expects 3 arguments that cannot be None/NULL.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(None, None, None, None, None)
      self.assertEqual(cm.exception.message, "Server raised fault: 'Missing parameter'")

   def test_03_no_document_model_id(self):
      """
      It is invalid to use the empty string as document level model ID.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context, '', '', '')
      self.assertEqual(cm.exception.message,
                       "Server raised fault: 'You must provide a valid document_model_id.'")


   def test_04_no_source_sentence(self):
      """
      The source sentence cannot be empty.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context,
                                             self.document_model_id, '', '')
      self.assertEqual(cm.exception.message,
                       "Server raised fault: 'You must provide a source sentence.'")


   def test_05_no_target_sentence(self):
      """
      The target sentence cannot be empty.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context,
                                             self.document_model_id, 
                                             self.source_sentence, '')
      self.assertEqual(cm.exception.message,
                       "Server raised fault: 'You must provide a target sentence.'")


   @unittest.skip("Should we check for too many parameters?")
   def test_06_too_many_parameters(self):
      """
      TODO: Should we get some sort of message if we provide an invalid number
      of arguments
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrAddSentence(self.context,
                                             self.document_model_id,
                                             self.source_sentence,
                                             self.target_sentence,
                                             'extra_dummy_argument')
      self.assertEqual(cm.exception.message, 
                       "Server raised fault: 'You must provide a target sentence.'")


   def test_07_basic_valid_usage(self):
      """
      This tests a valid call to incrAddSentence() where
      document_model_id is valid, source sentence is valid and target
      sentence is also valid.
      - The SOAP call should return true since it's supposed to be able to add
        this sentence pair to the queue.
      - The training phase should have inserted the sentence pair in the
        corpora.
      """
      UID = str(random.randint(0, 100000))
      source = self.source_sentence + str(time.time()) + UID
      target = self.target_sentence + str(time.time()) + UID
      result = self.client.service.incrAddSentence(self.context,
                                                   self.document_model_id,
                                                   source, target)
      self.assertEqual(result, True, 'SOAP call failed to add a sentence pair')

      r = requests.get(self.url + '/plive/DOCUMENT_MODEL_' + self.document_model_id + '/corpora')
      self.assertEqual(r.status_code, 200,
                       "Failed to fetch the corpora file for: " + self.document_model_id)

      ref_sentence_pair = '\t'.join((source, target))
      sentence_pairs = tuple(l.split('\t', 1)[-1] for l in r.text.split('\n'))
      self.assertEqual(sentence_pairs.count(ref_sentence_pair), 1,
                       "Expected exactly one occurrence of our sentence pair in corpora.")

      # Let incremental training finish.
      time.sleep(3);

      with open(os.path.join(self.document_model_dir, "incr-update.status"), "r") as sf:
         status = sf.read().strip()
      self.assertEqual(status, '0',
                       "0 exit status for incr-update.sh not found in incr-update.status.")

if __name__ == '__main__':
   unittest.main()

