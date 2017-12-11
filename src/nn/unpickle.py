#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file unpickle.py
# @brief Transform a pickle file from train-nnjm into a gzipped text file.
#
# @author Colin Cherry and Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014 - 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014 - 2017, Her Majesty in Right of Canada

"""
Transform a pickle file produced by train-nnjm into a human-readable gzipped
text file. This should be run immediately after train-nnjm, as the data
structures inside the nnjm pickle will not necessarily be unpickle-able in
later versions of Theano.

Format of layer is to have #input rows and #output columns

Hidden layers are listed with the bottom layer (connected to embeddings) first
and output layer last.

Version 1.1
"""

# Taken from portage_utils.py
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division
from __future__ import absolute_import
import __builtin__
from subprocess import Popen
from subprocess import PIPE

import sys
import cPickle
from argparse import ArgumentParser
import operator
import json
from functools import partial

import theano
import numpy
from theano import pp
from theano import tensor as T


class Layer:
   def __init__(self, w, b, act):
      self.w = w
      self.b = b
      self.act = act

class Embed:
   def __init__(self, voc_n, vec_size, words, embed):
      self.voc_n = voc_n
      self.vec_size = vec_size
      self.words = words
      self.embed = embed

def get_args():
   """Command line argument processing."""

   usage="unpickle [options] model.pkl model.out"
   help="""
   Transform a pickle file produced by train-nnjm into a human-readable
   gzipped txt file or a Portage binary file. Should be run immediately after
   train-nnjm, as the data structures inside the nnjm pickle will not
   necessarily be unpickle-able in later versions of theano

   Format of layer is to have #input rows and #output columns

   Hidden layers are listed with the bottom layer (connected to embeddings) first
   and output layer last.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   parser.add_argument("-h", "-help", "--help")

   parser.add_argument("-t", "--text", dest="textMode", action='store_true', default=False,
                       help="write in text format [%(default)s]")

   parser.add_argument("-d", "--describe", dest="describe", action='store_true', default=False,
                       help="describe the current model [%(default)s]")

   parser.add_argument("--summary", type=open, help="Blocks NNJM summary JSON file")

   parser.add_argument("modelIn", type=open, help="NNJM pickled model file.")

   parser.add_argument("modelOut", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output model file [sys.stdout]")

   cmd_args = parser.parse_args()
   return cmd_args

def loadBlocksNNJM(modelIn, jsummary):
   zfiles = numpy.load(modelIn)
   summary = json.load(jsummary)

   expected_in_size = 0

   # Source embedding, if any
   sembed = None
   if 'sources' in summary['embeds']:
      s = summary['sources']
      sembed = Embed(s['voc_size'],s['embed_size'],s['window_size'],zfiles[s['file']])
      expected_in_size += sembed.vec_size * sembed.words

   # Target embedding, if any
   tembed = None
   if 'targets' in summary['embeds']:
      s = summary['targets']
      tembed = Embed(s['voc_size'],s['embed_size'],s['window_size'],zfiles[s['file']])
      expected_in_size += tembed.vec_size * tembed.words

   # Hidden layers
   hlayers = [Layer(zfiles[s['files'][0]],zfiles[s['files'][1]],s['activation'])
              for s in summary['layers']]


   # Output is the last hidden layer
   out = hlayers[-1]
   hlayers = hlayers[:-1]
   out_voc_n = summary['layers'][-1]['output_dim']

   # Layer sanity check
   i=1
   for l in hlayers:
      if l.w.shape[0]!=expected_in_size:
         print("Error: expected input size {} for layer {} but got {}".format(expected_in_size,i,l.w.shape[0]))
         quit()
      expected_in_size = l.w.shape[1]
      i=i+1

   return sembed, tembed, hlayers, out_voc_n, out

def loadNNJMModel(modelIn):
   (stvec, ovec, out, sbed, tbed, st_x, hidden_layers) = cPickle.load(modelIn)

   # Output layer
   out = Layer(out.w.get_value(), out.b.get_value(), "none")
   o_voc_n = numpy.shape(out.w)[1]

   # Source embeddings
   s_embed = sbed.lookup.get_value()
   # Number of source words is size of source vector divided by size of embedding vector
   s_vec_size = numpy.shape(s_embed)[1]
   s_voc_n    = numpy.shape(s_embed)[0]
   s_words    = int(sbed.x_size / s_vec_size)

   # Target embeddings
   t_embed = tbed.lookup.get_value()
   # Number of target words is size of target vector divided by size of embedding vector
   t_vec_size = numpy.shape(t_embed)[1]
   t_voc_n    = numpy.shape(t_embed)[0]
   t_words    = int(tbed.x_size / t_vec_size)
   assert s_vec_size == t_vec_size, "source lookup size({}) is not the same as target lookup size({})".fomrat(s_vec_size, t_vec_size)

   # Hidden layers
   h = []
   # Layer 0 is bottom layer, connects to embeddings
   expected_in_size = (s_words+t_words)*s_vec_size
   for i,m in enumerate(hidden_layers):
      layer = Layer(m.w.get_value(), m.b.get_value(), m.output)
      h.append(layer)
      assert numpy.shape(layer.w)[0] == expected_in_size, "hidden layer {} error".format(i)
      expected_in_size = numpy.shape(layer.w)[1]
   # Layer -1 is top layer, connects to output
   assert numpy.shape(out.w)[0] == expected_in_size, "out size error"

   return s_voc_n, s_vec_size, s_words, sbed, t_voc_n, t_vec_size, t_words, tbed, hidden_layers, o_voc_n, out



def describeModelFile(modelIn):
   stvec, ovec, out, sbed, tbed, st_x, hidden_layers = cPickle.load(modelIn)
   describeModel(stvec, ovec, out, sbed, tbed, st_x, hidden_layers)



def describeModel(stvec, ovec, out, sbed, tbed, st_x, hidden_layers):
   def matrixSize(matrix):
      sizes = numpy.shape(matrix.get_value())
      return " x ".join(str(x) for x in sizes)

   print('Architecture')
   print('source input vector size: ', sbed.windowSize())
   print('target input vector size: ', tbed.windowSize())
   print('input vector size: ', sbed.windowSize() + tbed.windowSize())
   print('src win embedding size: ', sbed.x_size)
   print('tgt win embedding size: ', tbed.x_size)
   print('input layer size: ', sbed.x_size + tbed.x_size)
   for i, h in enumerate(hidden_layers):
      print("hidden layer", i+1, "size:", numpy.shape(h.w.get_value())[1], ", plus input bias" if h.b is not None else "")
   print("output layer size:", numpy.shape(out.w.get_value())[1], ", plus input bias" if out.b is not None else "")

   print('Weights')
   print("src embedding parameters: ", matrixSize(sbed.lookup))
   print("tgt embedding parameters: ", matrixSize(tbed.lookup))
   for i, h in enumerate(hidden_layers):
      print("hidden layer {} parameters: {}".format(i+1, matrixSize(h.w)))
      if h.b is not None:
         print('hidden_layer {} bias: {}'.format(i+1, matrixSize(h.b)))
   print('output layer parameters: ', matrixSize(out.w))
   print("output layer parameters: ", matrixSize(out.b))
   params = sbed.params + tbed.params + out.params
   for layer in hidden_layers:
      params += layer.params
   nparams = 0
   for p in params:
      nparams += reduce(operator.mul, p.get_value().shape, 1)
   print('total number or parameters: ', nparams)



def writeModelToFile(modelOut, textMode, source_embeddings, target_embeddings, hidden_layers, o_voc_n, out):
   with modelOut as f:
      myPrint = partial(print, file=f)

      def writeTextFile(source_embeddings, target_embeddings, hidden_layers, o_voc_n, out):
         """ Write to a text file."""
         def printStrVec(vec):
             """ Transform a vector to a string """
             myPrint(" ".join(['%.17f' % m for m in vec]))

         #Source embeddings
         if source_embeddings is not None:
            myPrint("[source]", source_embeddings.voc_n, source_embeddings.vec_size, source_embeddings.words)
            for vec in source_embeddings.embed:
               printStrVec(vec)

         #Target embeddings
         if target_embeddings is not None:
            myPrint("[target]", target_embeddings.voc_n, target_embeddings.vec_size, target_embeddings.words)
            for vec in target_embeddings.embed:
               printStrVec(vec)

         #Hidden layers
         for l in hidden_layers:
            myPrint("[hidden]", l.act, numpy.shape(l.w)[1])
            printStrVec(l.b)
            for vec in l.w:
               printStrVec(vec)

         #Output layer
         myPrint("[output]", "none", o_voc_n)
         printStrVec(out.b)
         for vec in out.w:
            printStrVec(vec)

      def writeBinFile(source_embeddings, target_embeddings, hidden_layers, o_voc_n, out):
         """ Write in binary format."""
         def printVectorToFile(vec):
            """ Make sure we write double for Portage."""
            vec.astype('float64').tofile(f, format='%d')

         #Source embeddings
         if source_embeddings is not None:
            myPrint("[source]", source_embeddings.voc_n, source_embeddings.vec_size, source_embeddings.words)
            printVectorToFile(source_embeddings.embed)

         #Target embeddings
         if target_embeddings is not None:
            myPrint("[target]", target_embeddings.voc_n, target_embeddings.vec_size, target_embeddings.words)
            printVectorToFile(target_embeddings.embed)

         #Hidden layers
         for l in hidden_layers:
            myPrint("[hidden]", l.act, numpy.shape(l.w)[1])
            printVectorToFile(l.b)
            printVectorToFile(l.w)

         #Output layer
         myPrint("[output]", "none", o_voc_n)
         printVectorToFile(out.b)
         printVectorToFile(out.w)


      if textMode:
         writeTextFile(source_embeddings, target_embeddings, hidden_layers, o_voc_n, out)
      else:
         writeBinFile(source_embeddings, target_embeddings, hidden_layers, o_voc_n, out)




def main():
   cmd_args = get_args()

   if cmd_args.describe:
      if cmd_args.summary is not None:
         print("Describe model not implemented for Blocks, model description is in {}".format(cmd_args.summary.name))
      else:
         describeModelFile(cmd_args.modelIn)
      exit()

   def loadModel(modelIn):
      s_voc_n, s_vec_size, s_words, sbed, t_voc_n, t_vec_size, t_words, tbed, hidden_layers, o_voc_n, out = loadNNJMModel(modelIn)
      s_embed = sbed.lookup.get_value()
      t_embed = tbed.lookup.get_value()
      h       = [Layer(m.w.get_value(), m.b.get_value(), m.output) for m in hidden_layers]
      return Embed(s_voc_n, s_vec_size, s_words, s_embed), Embed(t_voc_n, t_vec_size, t_words, t_embed), h, o_voc_n, out

   if cmd_args.summary is None:
      s, t, h, o_voc_n, out = loadModel(cmd_args.modelIn)
   else:
      s, t, h, o_voc_n, out = loadBlocksNNJM(cmd_args.modelIn, cmd_args.summary)

   writeModelToFile(cmd_args.modelOut, cmd_args.textMode, s, t, h, o_voc_n, out)



# Taken from portage_utils.py
def fatal_error(*args, **kwargs):
   """Print a fatal error message to stderr and exit with code 1."""
   print("Fatal error:", *args, file=sys.stderr, **kwargs)
   sys.exit(1)

# Taken from portage_utils.py
def open(filename, mode='r', quiet=True):
   """Transparently open files that are stdin, stdout, plain text, compressed or pipes.

   examples: open("-")
      open("file.txt")
      open("file.gz")
      open("zcat file.gz | grep a |")

   filename: name of the file to open
   mode: open mode
   quiet:  suppress "zcat: stdout: Broken pipe" messages.
   return: file handle to the open file.
   """
   filename.strip()
   #debug("open: ", filename, " in ", mode, " mode")
   if len(filename) is 0:
      fatal_error("You must provide a filename")

   if filename == "-":
      if mode == 'r':
         theFile = sys.stdin
      elif mode == 'w':
         theFile = sys.stdout
      else:
         fatal_error("Unsupported mode.")
   elif filename.endswith('|'):
      theFile = Popen(filename[:-1], shell=True, stdout=PIPE).stdout
   elif filename.startswith('|'):
      theFile = Popen(filename[1:], shell=True, stdin=PIPE).stdin
   elif filename.endswith(".gz"):
      #theFile = gzip.open(filename, mode+'b')
      if mode == 'r':
         if quiet:
            theFile = Popen(["zcat", "-f", filename], stdout=PIPE, stderr=open('/dev/null', 'w')).stdout
         else:
            theFile = Popen(["zcat", "-f", filename], stdout=PIPE).stdout
      elif mode == 'w':
         internal_file = __builtin__.open(filename, mode)
         theFile = Popen(["gzip"], close_fds=True, stdin=PIPE, stdout=internal_file).stdin
      else:
         fatal_error("Unsupported mode for gz files.")
   else:
      theFile = __builtin__.open(filename, mode)

   return theFile




if __name__ == '__main__':
   main()

## --Emacs trickery--
## Local Variables:
## mode:python
## python-indent:3
## End:
