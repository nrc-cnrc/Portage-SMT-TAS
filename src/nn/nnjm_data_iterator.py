#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# @file nnjm_data_iterator.py
# @brief Data Iterator for NNJM samples.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

from __future__ import print_function

import os
import sys
import tempfile
from subprocess import call
from subprocess import Popen
from subprocess import PIPE
from contextlib import contextmanager
from itertools import islice
import numpy as np
import numpy.random as rng

import logging


@contextmanager
def openShuffle(filename, mode = 'r', cleanup = True):
   """
   This function shuffles an input file and creates a temporary file for the
   intermediate shuffled data.  The intermediate shuffled file will
   automatically get deleted.
   """
   assert mode == 'r', 'You can only open a shuffled file in read mode.'

   shuffled_file, shuffled_filename = tempfile.mkstemp()
   os.close(shuffled_file)

   #print('openShuffle: Using {} as shuffled file.'.format(shuffled_filename))
   ret_code = call('gzip -cqdf {i} | shuf -o {o}'.format(i=filename, o=shuffled_filename), shell = True)
   if ret_code is not 0:
      print("Unable to shuffle the data file {}.".format(filename))
      sys.exit(1)

   try:
      # Let's use a context manager to make sure we close the file before deleting it in the finally block.
      with open(shuffled_filename, mode) as f:
         yield f
   finally:
      if shuffled_filename != None and cleanup:
         #print('openShuffle: Final removal of {}.'.format(shuffled_filename))
         os.remove(shuffled_filename)
         shuffled_filename = None



@contextmanager
def openShuffleNoTempFile(filename, mode = 'r'):
   """
   This open shuffle uses a pipe so there is no intermediate temporary file created.
   """
   try:
      process = Popen('gzip -cqdf {i} | shuf 2> /dev/null'.format(i=filename), shell=True, stdout=PIPE)
      #print(type(process))
      yield process.stdout
   finally:
      process.terminate()



def InfiniteIterator(filename, opener=open):
   """
   Infinitely read a file by constantly reopening it has needed.
   """
   #print(type(opener))
   while True:
      with opener(filename, 'r') as f:
         for l in f:
            yield l



def InfiniteShuffleIterator(filename, opener=open):
   """
   Infinitely read a file by constantly reopening it has needed.
   """
   shuffled_filename = None
   try:
      while True:
         shuffled_file, shuffled_filename = tempfile.mkstemp()
         os.close(shuffled_file)

         #print('Using {} as shuffled file.'.format(shuffled_filename))
         ret_code = call('gzip -cqdf {i} | shuf -o {o}'.format(i=filename, o=shuffled_filename), shell = True)
         if ret_code is not 0:
            print("Unable to shuffle the data file {}.".format(filename))
            sys.exit(1)

         with opener(shuffled_filename, 'r') as f:
            for l in f:
               yield l
         os.remove(shuffled_filename)
         shuffled_filename = None
   finally:
      if shuffled_filename != None:
         #print('Final removal of {}.'.format(shuffled_filename))
         os.remove(shuffled_filename)
         shuffled_filename = None




class DataIterator(object):
   """
   Creates mini-batches of block_size.
   """
   def __init__(self,
         iteratable,
         block_size = 4,
         swin_size = 11,
         thist_size = 3):
      """
      iteratable should yield a line of the follow form:
      2 2 2 2 2 13 184 439 254 5 197 / 2 2 2 / 28
      """
      self.iteratable = iteratable
      self.block_size = block_size
      self.swin_size = swin_size
      self.thist_size = thist_size

      self.thist_end = self.swin_size + 1 + self.thist_size
      self.usecols = range(self.swin_size) + range(self.swin_size+1, self.thist_end)

      # We sacrifice the first sample to validate the file format.
      s, t, o = self.iteratable.next().strip().split('/')
      assert len(s.split()) == self.swin_size, 'from file {}  from user {}'.format(len(s.split()), self.swin_size)
      assert len(t.split()) == self.thist_size
      assert len(o.split()) == 1


   @property
   def block_size(self):
      return self._block_size


   @block_size.setter
   def block_size(self, value):
      self._block_size = value


   def __iter__(self):
      return self


   def next(self):
      #data = np.asarray([ self.iteratable.next().strip().split() for _ in xrange(self.block_size) ])
      data = np.asarray([ l.strip().split() for l in islice(self.iteratable, self.block_size) ])
      if len(data) == 0:
         raise StopIteration
      x = np.asarray(data[:, self.usecols]).astype('int32')
      y = np.asarray(data[:, -1]).astype('int32')

      return x, y



class HardElideDataIterator(object):
   """
   Given a DataIterator, elide some target words from the history.
   """
   def __init__(self, iteratable, thist_size, thist_elide_size = 0):
      self.iteratable = iteratable
      self.thist_size = thist_size
      self.thist_elide_size = thist_elide_size

      assert self.thist_size == self.iteratable.thist_size,  'Error: Incompatible thist_sizes.'
      assert self.thist_elide_size <= self.thist_size, 'Error: You cannot elide more token than you have.'


   @property
   def block_size(self):
      return self.iteratable.block_size


   @block_size.setter
   def block_size(self, value):
      self.iteratable.block_size = value


   def __iter__(self):
      return self


   def next(self):
      x, y = self.iteratable.next()
      assert len(x) == len(y), 'Error: you should have the same number of X and y.'
      if self.thist_elide_size > 0:
         #logging.info("In read_datafile elide")
         #print("In read_datafile elide")
         if self.thist_elide_size < self.thist_size:
            x[:, -self.thist_size:-self.thist_size+self.thist_elide_size] = np.zeros(self.thist_elide_size)
         elif self.thist_elide_size == self.thist_size:
            x[:, -self.thist_size:] = np.zeros(self.thist_elide_size)
         else:
            assert False, 'thist_elide_size greater than thist_size'

      return (x, y)



class SoftElideDataIterator(object):
   def __init__(self, iteratable, thist_size, max_elide, elide_prob):
      self.iteratable = iteratable
      self.thist_size = thist_size
      self.max_elide  = max_elide
      self.elide_prob = elide_prob

      assert self.max_elide <= self.thist_size, 'You cannot elide more of the history than you have.'


   @property
   def block_size(self):
      return self.iteratable.block_size


   @block_size.setter
   def block_size(self, value):
      self.iteratable.block_size = value


   def __iter__(self):
      return self


   def next(self):
      x, y = self.iteratable.next()

      if self.max_elide<=0 or self.elide_prob<=0.0:
         return (x, y)

      for i in range(len(x)):
         if rng.uniform() < self.elide_prob:
            elide = rng.randint(self.max_elide + 1)
         else:
            elide = 0

         if elide > 0:
            if elide < self.thist_size:
               x[i, -self.thist_size:-self.thist_size+elide] = np.zeros(elide)
            elif elide == self.thist_size:
               x[i, -self.thist_size:] = np.zeros(elide)
            else:
               assert False, 'thist_elide_size greater than thist_size'

      return (x, y)
