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

import json
import re
import suds
import time
import unittest

from suds.client import Client


import logging
logging.basicConfig(format='%(asctime)-15s %(name)-8s %(message)s')
logging.getLogger('suds.client').setLevel(logging.CRITICAL)  # Set it to INFO to see the SOAP envelop that is sent.
logging.getLogger('suds.transport').setLevel(logging.CRITICAL)

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



class TestIncrAddSentence(unittest.TestCase):
    def setUp(self):
        self.c = Client('http://localhost/PortageLiveAPI.wsdl')
        self.context = 'incremental'
        self.doc_id = 'unittest_doc_id'
        self.source = 'This is a source sentence.'
        self.target = 'This is a target sentence.'
        self.extra_data = 'This is some extra data for my block.'


    def testAddSentence(self):
        self.assertTrue(self.c.service.incrAddSentence(self.context, self.doc_id, self.source, self.target, self.extra_data))



class TestIncrAddTextBlock(unittest.TestCase):
    """
    incrAddTextBlock() is used when we want to send multiple sentence
    source/target to be add to an incremental systems.
    """
    def setUp(self):
        self.c = Client('http://localhost/PortageLiveAPI.wsdl')
        self.context = 'incremental'
        self.doc_id = 'unittest_doc_id'
        self.source = 'This is a source block.'
        self.target = 'This is a target block.'
        self.extra_data = 'This is some extra data for my block.'

        self.big_source = """This is a bigger block.
        It has multiple lines.
        Three lines in total."""
        self.big_target = """Here's a multiline translation.
        We will make it this block longer than the source block.
        Hopefully, the aligner will to a great job.
        But, we are not helping it."""


    def testEmptyContext(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock( '', self.doc_id, self.source, self.target)
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a valid context.'")


    def testBadContext(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock('BAD_CONTEXT', self.doc_id, self.source, self.target)
        self.assertEqual(cm.exception.message, u"Server raised fault: 'Context \"BAD_CONTEXT\" does not exist.\n'")


    def testNoDocumentID(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock(self.context)
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a valid document_model_id.'")


    def testEmptyDocumentID(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock(self.context, '', self.source, self.target)
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a valid document_model_id.'")


    def testNoSource(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock(self.context, self.doc_id)
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a source text block.'")


    def testEmptySource(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock(self.context, self.doc_id, '')
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a source text block.'")


    def testNoTarget(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock(self.context, self.doc_id, self.source)
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a target text block.'")


    def testEmptyTarget(self):
        with self.assertRaises(suds.WebFault) as cm:
            self.c.service.incrAddTextBlock(self.context, self.doc_id, self.source, '')
        self.assertEqual(cm.exception.message, u"Server raised fault: 'You must provide a target text block.'")


    def testAddBlock(self):
        """
        Using `incrAddTextBlock()` with a simple sentence pair.
        """
        self.assertTrue(self.c.service.incrAddTextBlock(self.context, self.doc_id, self.source, self.target, self.extra_data))


    def testAddBigBlock(self):
        """
        Using `incrAddTextBlock()` with multiple sentence pairs.
        """
        self.assertTrue(self.c.service.incrAddTextBlock(self.context, self.doc_id, self.big_source, self.big_target, self.extra_data))


    #@unittest.skip("Skipping this test as it needs to wait for the server to finish incremental training which could take some time.")
    def testBlockEnd(self):
        """
	The intend of this test is to validate that `incrAddTextBlock()`
        proctects `__BLOCK_END__`, a special token used during the corpora alignment.
	We send a OOV `foobarbaz` with its translation `__BLOCK_END__`, wait
	for the server to finish its incremental update and then, we expect the
        translation of `foobarbaz` to be `__block_end_protected__`.
        """
        self.assertTrue(self.c.service.incrAddTextBlock(self.context, self.doc_id, 'foobarbaz', '__BLOCK_END__', 'unittest: guarding __BLOCK_END__'))
        time.sleep(10)
        translation = self.c.service.translate('foobarbaz', self.context + '/' + self.doc_id, 's')
        #print(translation)
        self.assertEqual(translation['Result'].strip(), '__block_end__')
