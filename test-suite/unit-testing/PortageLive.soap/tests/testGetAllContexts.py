#!/usr/bin/env python

# @file testGetAllContexts.py
# @brief Test SOAP calls to getAllContexts using a deployed PortageLive web server.
#
# @author Eric Joanis & Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

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
import json

logging.basicConfig(level=logging.CRITICAL)
# If you need to debug what is happening, uncomment the following line
#logging.basicConfig(level=logging.DEBUG)

url = 'http://127.0.0.1'

class TestGetAllContexts(unittest.TestCase):
   """
   Using PortageLiveAPI's WSDL deployed on a web server, test SOAP calls to
   getAllContexts().
   """

   def __init__(self, *args, **kwargs):
      super(TestGetAllContexts, self).__init__(*args, **kwargs)
      self.longMessage = True

      DocumentCache().clear()
      self.url = url + ':' + os.getenv('PHP_PORT', 8756)
      self.WSDL = self.url + '/PortageLiveAPI.wsdl'
      self.client = Client(self.WSDL)

   def setUp(self):
      return

   def tearDown(self):
      if (os.path.lexists('doc_root/tests')):
         os.unlink('doc_root/tests')
      os.symlink('../tests','doc_root/tests')

   def test_01_get_all_contexts(self):
      """
      Call getAllContexts() soap function
      """
      result = self.client.service.getAllContexts(False, False)
      expected = 'unittest.rev.en-fr'
      self.assertEqual(result, expected,
         "getAllContexts() should return unittest.rev.en-fr")

   def test_02_get_all_contexts_verbose(self):
      """
      Call getAllContexts() soap function with verbose output
      """

      result = self.client.service.getAllContexts(True, False)
      expected = 'unittest.rev.en-fr ( --> )';
      self.assertEqual(result, expected,
         "getAllContexts(1) should return unittest.rev.en-fr and its description")

   def test_03_get_all_contexts_json(self):
      """
      Call getAllContexts() soap function with JSON output
      """
      json_string = self.client.service.getAllContexts(True, True)
      #print(json_string)
      result = json.loads(json_string)
      #print(result)
      #print(result[0]['source'])

      self.assertEqual(len(result), 1, "There should only be one system")
      #self.assertEqual(len(result), 2, "There are not two systems")

      expected = {u'contexts':[{
         u'source': u'en',
         u'description': u'unittest.rev.en-fr ( --> )',
         u'name': u'unittest.rev.en-fr',
         u'target': u'fr'
      }]}
      self.assertEqual(result, expected, "Json structure has elements we expect")
      #expected = [{
      #   u'description': u'unittest.rev.en-fr ( --> )',
      #   u'nome': u'unittest.rev.en-fr',
      #   u'source': u'en',
      #   u'target': u'fr'
      #}]
      #self.assertEqual(result, expected, "Json structure with intentional errors")

   def run_multi_get_all_contexts(self, scenario,
                                  base_expected, verbose_expected, json_expected):
      if (os.path.lexists('doc_root/tests')):
         os.unlink('doc_root/tests')
      os.symlink('../scenarios/'+scenario, 'doc_root/tests')

      result = self.client.service.getAllContexts(False, False)
      self.assertEqual(result, base_expected,
         'Called getAllContexts() on scenario {}'.format(scenario))

      result = self.client.service.getAllContexts(True, False)
      self.assertEqual(result, verbose_expected,
         'Called getAllContexts(verbose) on scenario {}'.format(scenario))

      result = json.loads(self.client.service.getAllContexts(True, True))
      self.assertEqual(result, json_expected,
         'Called getAllContexts(json) on scenario {}'.format(scenario))

   def test_04_no_contexts(self):
      """
      call getAllContexts() is a case where there are no contexts.
      """
      self.run_multi_get_all_contexts('no_contexts', None, None, {u'contexts':[]})

   def test_05_invalid_scenario(self):
      """
      call getAllContexts() is a case where the models directory does not exist
      """
      self.run_multi_get_all_contexts('invalid_scenario', None, None, {u'contexts':[]})

   def test_06_one_context(self):
      """
      call getAllContexts() is a case where there is exactly one context
      """
      self.run_multi_get_all_contexts('one_context', 'unittest.rev.en-fr',
         'unittest.rev.en-fr ( --> )',
         {u'contexts':
         [{u'description': u'unittest.rev.en-fr ( --> )',
           u'name':     u'unittest.rev.en-fr',
           u'source':   u'en',
           u'target':   u'fr'}]})

   def test_07_several_contexts(self):
      """
      call getAllContexts() is a case where there are several contexts
      """
      self.run_multi_get_all_contexts('several_contexts',
         'toy-regress-ch2en;toy-regress-en2fr;toy-regress-en2fr.nnjm;toy-regress-fr2en;unittest.rev.en-fr',
         'toy-regress-ch2en (CH-CA --> EN-CA);toy-regress-en2fr (EN-CA --> FR-CA) with CE;toy-regress-en2fr.nnjm (EN-CA --> FR-CA) with CE;toy-regress-fr2en (FR-CA --> EN-CA) with CE;unittest.rev.en-fr ( --> )',
         {u'contexts':
         [{u'source': u'en', u'description': u'toy-regress-ch2en (CH-CA --> EN-CA)', u'name': u'toy-regress-ch2en', u'target': u'fr'},
          {u'source': u'en', u'description': u'toy-regress-en2fr (EN-CA --> FR-CA) with CE', u'name': u'toy-regress-en2fr', u'target': u'fr'},
          {u'source': u'en', u'description': u'toy-regress-en2fr.nnjm (EN-CA --> FR-CA) with CE', u'name': u'toy-regress-en2fr.nnjm', u'target': u'fr'},
          {u'source': u'en', u'description': u'toy-regress-fr2en (FR-CA --> EN-CA) with CE', u'name': u'toy-regress-fr2en', u'target': u'fr'},
          {u'source': u'en', u'description': u'unittest.rev.en-fr ( --> )', u'name': u'unittest.rev.en-fr', u'target': u'fr'}]})

   #TODO
   # - no contexts, one context, more than one context
   # - default vs verbose vs json
