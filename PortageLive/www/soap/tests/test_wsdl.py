#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file test_wsdl.py
# @brief Python unittest for PortageLive's WSDL.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2018, Sa Majeste la Reine du Chef du Canada
# Copyright 2018, Her Majesty in Right of Canada

# python -m unittest tests.test_wsdl
# python -m unittest tests.test_wsdl.TestGetAllContexts
# python -m unittest tests.test_wsdl.TestGetAllContexts.testJson01
# python -m unittest discover -s tests -p 'test*.py'

from __future__ import print_function

import unittest
import json
from suds.client import Client
import re

#import logging
#logging.getLogger('suds.transport').setLevel(logging.DEBUG)

#####################################
# Suds has its own cache /tmp/suds/*
# rm /tmp/suds/*
# rm /var/lib/php/wsdlcache/
#####################################
from suds.cache import DocumentCache
DocumentCache().clear()




class TestGetAllContexts(unittest.TestCase):
   """
   Unittest for WSDL's getAllContexts().
   """
   def __init__(self, *args, **kwargs):
      super(TestGetAllContexts, self).__init__(*args, **kwargs)

      self.c = Client('http://localhost/PortageLiveAPI.wsdl')
      # ADMIN.e2f (EN-CA --> FR-CA) [Incr]
      self.re = re.compile(r'.+ \(([-a-zA-Z]+ --> [-a-zA-Z])|\w+\)( [Incr])?')


   def testString01(self):
      """
      getAllContexts() not in verbose mode and not in json.
      Note that I'm not sure what to validate for this request?
      """
      response = self.c.service.getAllContexts(verbose=False, json=False)
      contexts = response.strip().split(';')
      self.assertTrue(len(contexts) > 0, 'You should get at least one context.  Are you using a valid server?')


   def testString02(self):
      """
      getAllContexts() in verbose mode but not in json.
      Expected format for one context:
      ADMIN.e2f (EN-CA --> FR-CA) [Incr]
      """
      response = self.c.service.getAllContexts(verbose=True, json=False)
      contexts = response.strip().split(';')
      self.assertTrue(len(contexts) > 0, 'You should get at least one context.  Are you using a valid server?')
      for m in contexts:
         self.assertTrue(self.re.match(m), 'Invalid context format: {}'.format(m))


   def testJson01(self):
      """
      getAllContexts() in json mode but not in verbose mode.
      This is the expected json response's format:
      {
         u'source': u'EN-CA',
         u'target': u'FR-CA',
         u'is_incremental': False,
         u'name': u'ADMIN.e2f',
         u'description': u'ADMIN.e2f (EN-CA --> FR-CA) [Incr]'
      }
      """
      response = self.c.service.getAllContexts(verbose=False, json=True)
      response = json.loads(response)
      contexts = response['contexts']
      self.assertTrue(len(contexts) > 0, 'You should get at least one context.  Are you using a valid server?')
      for c in contexts:
         self.assertTrue('source' in c.viewkeys(), 'Missing source')
         self.assertTrue('target' in c.viewkeys(), 'Missing target')
         self.assertTrue('name' in c.viewkeys(), 'Missing name')
         self.assertTrue('description' in c.viewkeys(), 'Missing description')
         self.assertTrue('is_incremental' in c.viewkeys(), 'Missing is_incremental')


   def testJson02(self):
      """
      getAllContexts() in json mode and also in verbose mode.
      A verbose json response is the same as a none verbose one.
      """
      # This part is not verbose.
      response = self.c.service.getAllContexts(verbose=False, json=True)
      response = json.loads(response)
      contexts = response['contexts']
      self.assertTrue(len(contexts) > 0, 'You should get at least one context.  Are you using a valid server?')

      # This part is in verbose mode.
      response_verbose = self.c.service.getAllContexts(verbose=True, json=True)
      response_verbose = json.loads(response_verbose)
      contexts_verbose = response_verbose['contexts']
      self.assertTrue(len(contexts_verbose) > 0, 'You should get at least one context.  Are you using a valid server?')

      # There should be no difference between json verbose and json not verbose.
      self.assertEquals(cmp(contexts, contexts_verbose), 0, 'Both json verbose or not requests should be equal.')
