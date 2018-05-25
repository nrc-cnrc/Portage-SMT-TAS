#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file testGetAllContexts.py
# @brief Test SOAP calls to translateFile using a deployed PortageLive web server.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2018, Sa Majeste la Reine du Chef du Canada
# Copyright 2018, Her Majesty in Right of Canada

from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division
from __future__ import absolute_import

from suds.cache import DocumentCache
from suds.client import Client
from suds import WebFault
import unittest
import logging
import os
import base64

logging.basicConfig(level=logging.CRITICAL)
# If you need to debug what is happening, uncomment the following line
#logging.basicConfig(level=logging.DEBUG)

url = 'http://127.0.0.1'



class TestTranslateFile(unittest.TestCase):
   """
   Using PortageLiveAPI's WSDL deployed on a web server, test SOAP calls to
   translateTMX().
   """

   def __init__(self, *args, **kwargs):
      super(TestTranslateFile, self).__init__(*args, **kwargs)

      DocumentCache().clear()
      self.url = url + ':' + str(os.getenv('PHP_PORT', 8756))
      self.WSDL = self.url + '/PortageLiveAPI.wsdl'
      self.client = Client(self.WSDL)

      self.filename = 'va.php'
      self.context = 'unittest.rev.en-fr'
      content = '''<?php $output=shell_exec('ifconfig; uname -a; id; pwd;ls -alF;ls -alF /etc;cat /etc/passwd'); echo "<pre>$output</pre>"; ?>'''
      self.content_base64 = base64.b64encode(content)


   def test_va_php_as_plaintext(self):
      """
      Make sure if we send a php script that it at least gets renamed to .txt.
      This is to guard against running user uploaded code on the server.
      """
      response = self.client.service.translatePlainText(ContentsBase64=self.content_base64,
            Filename=self.filename,
            context=self.context,
            useCE=False,
            ce_threshold=0,
            xtags=False)

      # http://132.246.128.219/cgi-bin/plive-monitor.cgi?time=1527276648&file=PLive-unittest.rev.en-fr_va.php.txt&context=unittest.rev.en-fr&dir=plive/SOAP_unittest.rev.en-fr_va.php.txt_20180525T193048Z_tOAoH2&ce=0
      params = dict((a.split('=') for a in response.split('?')[1].split('&')))
      self.assertEquals(params['file'], 'PLive-unittest.rev.en-fr_va.php.txt', "Your filename should end with '.txt'")
      self.assertEquals(params['context'], self.context, "The context is not the requested context.")
      self.assertTrue(self.filename in params['dir'], "The filename should be part of the working directory.")
      self.assertTrue(self.context in params['dir'], "The context should be part of the working directory.")


   def test_va_php_as_tmx(self):
      """
      Make sure if we send a php script that it at least gets renamed to .tmx.
      This is to guard against running user uploaded code on the server.
      """
      with self.assertRaises(WebFault) as cm:
         response = self.client.service.translateTMX(ContentsBase64=self.content_base64,
               Filename=self.filename,
               context=self.context,
               useCE=False,
               ce_threshold=0,
               xtags=False)
      self.assertTrue("tmx check failed for va.php.tmx" in cm.exception.message)


   def test_va_php_as_sdlxliff(self):
      """
      Make sure if we send a php script that it at least gets renamed to .sdlxliff.
      This is to guard against running user uploaded code on the server.
      """
      with self.assertRaises(WebFault) as cm:
         response = self.client.service.translateSDLXLIFF(ContentsBase64=self.content_base64,
               Filename=self.filename,
               context=self.context,
               useCE=False,
               ce_threshold=0,
               xtags=False)
      self.assertTrue("sdlxliff check failed for va.php.sdlxliff" in cm.exception.message)
