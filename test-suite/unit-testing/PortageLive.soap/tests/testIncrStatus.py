#!/usr/bin/env python

# @file testIncrStatus.py
# @brief Test SOAP calls to incrStatus using a deployed PortageLive web server.
#
# @author Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division
from __future__ import absolute_import

from suds.cache import DocumentCache
from suds.client import Client
from suds import WebFault
import unittest
import logging
import requests
import sys
import time
import os
import random
import shutil
import glob

logging.basicConfig(level=logging.CRITICAL)
# If you need to debug what is happening, uncomment the following line
#logging.basicConfig(level=logging.DEBUG)

url = 'http://127.0.0.1'



class TestIncrStatus(unittest.TestCase):
   """
   Using PortageLiveAPI's WSDL deployed on a web server, test SOAP calls to
   incrStatus().
   """

   def __init__(self, *args, **kwargs):
      super(TestIncrStatus, self).__init__(*args, **kwargs)
      self.longMessage = True

      DocumentCache().clear()
      self.url = url + ':' + os.getenv('PHP_PORT', 8756)
      self.WSDL = self.url + '/PortageLiveAPI.wsdl'
      self.client = Client(self.WSDL)
      self.context = 'unittest.rev.en-fr'
      self.document_model_id = 'PORTAGE_UNITTEST_4da35_2'
      self.source_sentence = "'home'"
      self.target_sentence = '"maison"'

      self.document_model_dir = os.path.join("doc_root", "plive",
                                    "DOCUMENT_MODEL_"+self.context+'_'+self.document_model_id)
      document_model_dirs = glob.glob(self.document_model_dir+"*")
      for dir in document_model_dirs:
         if (os.path.isdir(dir)): shutil.rmtree(dir)


   def test_01_no_argument(self):
      """
      incrStatus() should warn the user that it needs some parameters.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrStatus()
      expected = "Server raised fault: 'Missing parameter'"
      self.assertEqual(cm.exception.message, expected)


   def test_02_all_arguments_null(self):
      """
      incrStatus() expects 1 argument, which cannot be None/NULL.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrStatus(None)
      expected = "Server raised fault: 'Missing parameter'"
      self.assertEqual(cm.exception.message, expected)


   def test_03_no_document_model_id(self):
      """
      It is invalid to use the empty string as document model ID.
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrStatus(self.context, '')
      expected = "Server raised fault: 'You must provide a valid document_model_id.'"
      self.assertEqual(cm.exception.message, expected)


   def test_04_nonexistent_document_model_id(self):
      """
      Expect 'N/A' for a non-existent document model ID.
      """
      result = self.client.service.incrStatus(self.context, self.document_model_id)
      self.assertEqual(result, "N/A",
         'Unexpected output from incrStatus SOAP call for a non-existent document model id')


   def test_05_empty_document_model(self):
      """
      Expect 'N/A' for an empty document model directory.
      """
      os.mkdir(self.document_model_dir)

      result = self.client.service.incrStatus(self.context, self.document_model_id)
      self.assertEqual(result, "N/A",
         'Unexpected output from incrStatus SOAP call for an empty document model')


   @unittest.skip("Should we check for too many parameters?")
   def test_06_too_many_parameters(self):
      """
      TODO: Should we get some sort of message if we provide an invalid number
      of arguments
      """
      with self.assertRaises(WebFault) as cm:
         self.client.service.incrStatus(self.context, self.document_model_id, 'extra_dummy_argument')
      self.assertEqual(cm.exception.message, "Server raised fault: 'Too many arguments.'")


   def test_07_one_incr_training(self):
      """
      This tests calls to incrStatus() during a single round of incremental
      training.
      """
      document_model_id = self.document_model_id + ".1"

      UID = str(random.randint(0, 100000))
      source = self.source_sentence + str(time.time()) + UID
      target = self.target_sentence + str(time.time()) + UID
      result = self.client.service.incrAddSentence(self.context, document_model_id, source, target)
      self.assertEqual(result, True, 'SOAP call failed to add a sentence pair')

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update in_progress, N/A, corpus: 1, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call for training in progress.")

      # Let incremental training finish.
      time.sleep(3);

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update complete, 0 success, corpus: 1, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call after training is completed.")


   def test_08_two_incr_training(self):
      """
      This tests calls to incrStatus() during two rounds of incremental
      training, with an additional sentence pair added during the first round
      of training.
      """
      document_model_id = self.document_model_id + ".2"

      uid = str(random.randint(0, 100000))
      source = self.source_sentence + str(time.time()) + uid
      target = self.target_sentence + str(time.time()) + uid
      result = self.client.service.incrAddSentence(self.context, document_model_id, source, target)
      self.assertEqual(result, True, 'SOAP call failed to add a sentence pair')

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update in_progress, N/A, corpus: 1, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call after first sentence pair added.")

      uid = str(random.randint(0, 100000))
      source = self.source_sentence + str(time.time()) + uid
      target = self.target_sentence + str(time.time()) + uid
      result = self.client.service.incrAddSentence(self.context, document_model_id, source, target)
      self.assertEqual(result, True, 'SOAP call failed to add a sentence pair')

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update pending+in_progress, N/A, corpus: 1, queue: 1"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call after second sentence pair added.")

      # Let first incremental training finish.
      time.sleep(3);

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update in_progress, 0 success, corpus: 2, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call after first training completed.")

      # Let second incremental training finish.
      time.sleep(3);

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update complete, 0 success, corpus: 2, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call after second training completed.")


   def test_09_failure_status(self):
      """
      This tests calls to incrStatus() during a single round of incremental
      training.
      """
      document_model_id = self.document_model_id + ".3"

      UID = str(random.randint(0, 100000))
      source = self.source_sentence + str(time.time()) + UID
      target = self.target_sentence + str(time.time()) + UID
      result = self.client.service.incrAddSentence(self.context, document_model_id, source, target)
      self.assertEqual(result, True, 'SOAP call failed to add a sentence pair')

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update in_progress, N/A, corpus: 1, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call for training in progress.")

      # Let incremental training finish.
      time.sleep(3);

      # Fake a failure.
      with open(os.path.join(self.document_model_dir+".3", "incr-update.status"), "w") as sf:
         print("1", file=sf)

      result = self.client.service.incrStatus(self.context, document_model_id)
      expected = "Update complete, 1 failure, corpus: 1, queue: 0"
      self.assertEqual(result, expected,
         "Unexpected output from incrStatus SOAP call after failed training is completed.")




if __name__ == '__main__':
   unittest.main()

