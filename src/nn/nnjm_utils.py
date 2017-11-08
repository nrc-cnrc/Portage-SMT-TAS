#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file nnjm_utils.py
# @brief Utilities for NNJM training.
#
# @author George Foster
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

import sys, os
import numpy
import mmap
import gzip

verbose = 1

def error(msg):
   print >> sys.stderr, "Error:", msg
   sys.exit(1)

def warn(msg):
   print >> sys.stderr, "Warning:", msg

def log(msg, *other_vars):
   if verbose:
      print >> sys.stderr, msg,
      for m in other_vars: print >> sys.stderr, m,
      print >> sys.stderr

def read_datafile(filename, swin_size, thist_size, thist_elide_size):
   """Read training file into numpy array"""
   thist_end = swin_size+1+thist_size
   x = numpy.loadtxt(filename, dtype=numpy.int32, usecols=range(swin_size)+range(swin_size+1, thist_end))
   y = numpy.loadtxt(filename, dtype=numpy.int32, usecols=range(thist_end+1, thist_end+2))
   log("read %d examples from %s" % (len(x), filename))
   assert(len(x) == len(y))
   if thist_elide_size > 0:
      log("In read_datafile elide")
      if thist_elide_size < thist_size:
         x[:, -thist_size:-thist_size+thist_elide_size] = numpy.zeros(thist_elide_size)
      elif thist_elide_size == thist_size:
         x[:, -thist_size:] = numpy.zeros(thist_elide_size)
      else:
         error("thist_elide_size greater than thist_size")
   return (x, y)



def line_count(filename):
   with gzip.open(filename, "r") as f:
      lines = 0
      for _ in f:
         lines += 1
      return lines
