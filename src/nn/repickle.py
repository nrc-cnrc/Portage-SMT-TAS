#!/usr/bin/env python
# @file repickle.py
# @brief Converts a binary NNJM model file to a pickle NNJM model file.
#
# @author Samuel Larkin
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2018, Sa Majeste la Reine du Chef du Canada
# Copyright 2018, Her Majesty in Right of Canada


from __future__ import print_function

import sys
import os

import cPickle as pickle
import numpy as np

# MKL_THREADING_LAYER=GNU is always required, so set it in here, before importing Theano
os.environ["MKL_THREADING_LAYER"] = "GNU"

from theano import shared, tensor as T

from argparse import ArgumentParser

from embed import EmbedLayer
from multilayers import HiddenLayer
from multilayers import OutputLayer



def readEmbeddings(layer_name, model_fh, stvec, start=0, embed_bias = False):
   """
   Reads and create an EmbedLayer.
   """
   header = model_fh.readline()
   name, svoc_size, embed_size, swin_size = header.strip().split()
   assert name == '[{}]'.format(layer_name), "Invalid format, we were expecting the {} embedding layer.".format(layer_name)
   svoc_size, embed_size, swin_size = int(svoc_size), int(embed_size), int(swin_size)
   W = np.fromfile(model_fh, dtype='float64', count=svoc_size * embed_size) \
         .reshape((svoc_size, embed_size)) \
         .astype('float32')

   sbed = EmbedLayer(stvec, swin_size, svoc_size, embed_size, start, embed_bias) # src win embeddings: N x (swin_size*embed_size)
   sbed.lookup.set_value(W)

   return sbed, svoc_size, embed_size, swin_size


def readParameters(model_fh, n_in, n_out):
   """
   Reads the binary form of the bias and weights vector/matrix.
   """
   b = np.fromfile(model_fh, dtype='float64', count=n_out) \
         .reshape((n_out,)) \
         .astype('float32')
   W = np.fromfile(model_fh, dtype='float64', count=n_in * n_out) \
         .reshape((n_in, n_out)) \
         .astype('float32')

   return b, W


def convertActivation(activation_name):
   # Elemwise{tanh,no_inplace}.0
   if 'tanh' in activation_name:
      return T.tanh
   else:
      error('Unknown activation name {}'.format(activation_name))


def loadBinModel(name, embed_bias = False, hidden_bias = True, output_bias = True):
   """
   Load a NNJM binary model aka the output of unpickle.py.
   Return:
      - stvec a T.imatrix of shape src win + tgt hist vectors: N x (swin_size + thist_size)
      - ovec a T.ivector of shape N x 1
      - out the OutputLayer
      - sbed the source EmbedLayer
      - tbed the target EmbedLayer
      - st_x the concatenation of sbed and tbed
      - hidden_layers a list of HiddenLayer
   """
   stvec = T.imatrix("stvec") # src win + tgt hist vectors: N x (swin_size + thist_size)
   ovec  = T.ivector("ovec") # output classes: N x 1

   with open(name, 'rb') as model_fh:
      sbed, svoc_size, embed_size, swin_size = readEmbeddings('source', model_fh, stvec, 0, embed_bias = embed_bias)
      tbed, tvoc_size, embed_size, twin_size = readEmbeddings('target', model_fh, stvec, swin_size, embed_bias = embed_bias)
      st_x = T.concatenate([sbed.x, tbed.x], 1) # combined embedding

      hidden_layers = []
      last_x = st_x
      last_n = sbed.x_size + tbed.x_size
      header = model_fh.readline()
      while header.startswith('[hidden]'):
         name, activation, hidden_size = header.strip().split()
         hidden_size = int(hidden_size)
         b, W = readParameters(model_fh, last_n, hidden_size)

         hidden_layer = HiddenLayer(last_x, last_n, hidden_size, convertActivation(activation), hidden_bias)
         hidden_layer.w.set_value(W)
         hidden_layer.b.set_value(b)
         hidden_layers.append(hidden_layer)
         last_x, last_n = hidden_layer.output, hidden_size
         header = model_fh.readline()

      assert header.startswith('[output]'), "Invalid format, expected an output layer."
      name, activation, ovoc_size = header.strip().split()
      ovoc_size = int(ovoc_size)
      assert activation == 'none', 'Wrong expected activation for the output layter.'
      b, W = readParameters(model_fh, last_n, ovoc_size)
      out = OutputLayer(last_x, ovec, last_n, ovoc_size, output_bias)
      out.b.set_value(b)
      out.w.set_value(W)

      assert model_fh.read() == '', 'The binary file was improperly read.'

   return stvec, ovec, out, sbed, tbed, st_x, hidden_layers



def test():
   """
   A function for manually testing that loading a nnjm.bin is the same as
   loading the same model in pickle format.
   This could be a Unittest but I'm afraid that the models would play well with
   our Git.
   """
   name = 'pretrained.nnjm.generic-2.0.en2fr.mm/nnjm.bin'
   stvec, ovec, out, sbed, tbed, st_x, hidden_layers = loadBinModel(name)

   import cPickle
   with open(name[:-4] + '.pkl') as model_fh:
      ref_stvec, ref_ovec, ref_out, ref_sbed, ref_tbed, ref_st_x, ref_hidden_layers = cPickle.load(model_fh)
   print('=====================')
   print(sbed.lookup.get_value().shape)
   print(ref_sbed.lookup.get_value().shape)
   assert np.allclose(sbed.lookup.get_value(), ref_sbed.lookup.get_value())

   print(tbed.lookup.get_value().shape)
   print(ref_tbed.lookup.get_value().shape)
   assert np.allclose(tbed.lookup.get_value(), ref_tbed.lookup.get_value())

   print(hidden_layers[0].b.get_value().shape)
   print(ref_hidden_layers[0].b.get_value().shape)
   assert np.allclose(hidden_layers[0].b.get_value(), ref_hidden_layers[0].b.get_value())

   print(hidden_layers[0].w.get_value().shape)
   print(ref_hidden_layers[0].w.get_value().shape)
   assert np.allclose(hidden_layers[0].w.get_value(), ref_hidden_layers[0].w.get_value())

   print(out.b.get_value().shape)
   print(ref_out.b.get_value().shape)
   assert np.allclose(out.b.get_value(), ref_out.b.get_value())

   print(out.w.get_value().shape)
   print(ref_out.w.get_value().shape)
   assert np.allclose(out.w.get_value(), ref_out.w.get_value())



# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path)
# to arrange that portage_utils will be imported from src/utils.
if sys.argv[0] not in ('', '-c'):
   bin_path = os.path.dirname(sys.argv[0])
   if os.path.basename(bin_path) != "bin":
      sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

# portage_utils provides a bunch of useful and handy functions, including:
#   HelpAction, VerboseAction, DebugAction (helpers for argument processing)
#   printCopyright
#   info, verbose, debug, warn, error, fatal_error
#   open (transparently open stdin, stdout, plain text files, compressed files or pipes)
from portage_utils import *

def get_args():
   """Command line argument processing."""

   usage="repickle  binary_model_filename pickle_model_filename"
   help="""
   Given a binary NNJM model file, converts it to a pickle file for train-nnjm.py -pretrain_model.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   # The following use the portage_utils version of open to open files.
   parser.add_argument("binary_model", type=str, help="input file [sys.stdin]")
   parser.add_argument("pickle_model", type=str, help="output file [sys.stdout]")

   return parser.parse_args()



def main():
   args = get_args()
   with open(args.pickle_model, 'wb') as f:
      pickle.dump(loadBinModel(args.binary_model), f, protocol=pickle.HIGHEST_PROTOCOL)



if __name__ == '__main__':
   main()
