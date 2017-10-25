#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file train-nnjm.py
# @brief Train an NNJM model.
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
import numpy.random as rng
import theano
import theano.tensor as T
import msgd, multilayers, embed
import nnjm_utils
from nnjm_utils import *
from multilayers import HiddenLayer, OutputLayer
from embed import EmbedLayer
import argparse
from collections import Counter
import operator
import time
import cPickle
import copy

rng.seed(1)

# Functions

def gen_rnd_data(N, swin_size, thist_size, svoc_size, tvoc_size, ovoc_size):
   if N == 0: return None
   return (numpy.concatenate(
           (rng.randint(size=(N,swin_size), low=0, high=svoc_size),
            rng.randint(size=(N,thist_size), low=0, high=tvoc_size)), 1).astype(numpy.int32),
            rng.randint(size=N, low=0, high=ovoc_size).astype(numpy.int32))



def rnd_elide(D, thist_size, max_elide, elide_prob):
   assert max_elide <= thist_size

   if max_elide<=0 or elide_prob<=0.0 :
      return D

   x = copy.deepcopy(D[0])
   y = D[1]
   for i in range(len(x)):

      if rng.uniform() < elide_prob:
         elide = rng.randint(max_elide+1)
      else:
         elide = 0

      if elide>0:
         if elide < thist_size:
            x[i,-thist_size:-thist_size+elide]=numpy.zeros(elide)
         elif elide==thist_size:
            x[i,-thist_size:]=numpy.zeros(elide)
         else:
            assert False

   return (x,y)



def printStatusInfo(args, params, stvec, ovec, sbed, tbed, st_x, hidden_layers, out, D):
   log("Parameter settings:")
   for a in sorted(vars(args).keys()):
      log('... ' + a + '=' + str(vars(args)[a]))

   def dim_str(shared_matrix):
      s = shared_matrix.get_value().shape
      ret = ""
      for i in range(len(s)):
         if ret != "": ret += " x "
         ret += str(s[i])
      return ret

   log("Architecture:")
   f = theano.function([stvec, ovec],
      [stvec[0], sbed.x[0], tbed.x[0], st_x[0]]
      + [h.output[0] for h in hidden_layers]
      + [out.log_p_y_given_x[0], out.nll])
   test_out = f([D[0][0]],[D[1][0]])
   log("... input vector size:", len(test_out[0]))
   log("... src win embedding size:", len(test_out[1]), ", plus input bias" if args.embed_bias else "")
   log("... tgt hist embedding size:", len(test_out[2]), ", plus input bias" if args.embed_bias else "")
   log("... input layer size:", len(test_out[3]))
   for i in range(len(hidden_layers)):
      log("... hidden layer", i+1, "size:", len(test_out[4+i]), ", plus input bias" if args.hidden_bias else "")
   log("... output layer size:", len(test_out[4+len(hidden_layers)]), ", plus input bias" if args.output_bias else "")

   log("Weights:")
   pos = 0
   log("... src embedding parameters:", dim_str(params[pos]))
   pos+=1
   if args.embed_bias:
      log("... src embedding bias:", dim_str(params[pos]))
      pos+=1
   log("... tgt embedding parameters:", dim_str(params[pos]))
   pos+=1
   if args.embed_bias:
      log("... tgt embedding bias:", dim_str(params[pos]))
      pos+=1
   for i in range(len(hidden_layers)):
      log("... hidden layer", i+1, "parameters:", dim_str(params[pos]))
      if args.hidden_bias:
         log("... hidden_layer", i+1, "bias:", dim_str(params[pos+1]))
         pos += 1
      pos +=1
   log("... output layer parameters:", dim_str(params[pos]))
   if args.output_bias:
      log("... output layer bias:", dim_str(params[pos+1]))
   nparams = 0
   for p in params:
      nparams += reduce(operator.mul, p.get_value().shape, 1)
   log("... total number of parameters:", nparams)



def train(args):
   nnjm_utils.verbose = args.v # for log
   hidden_layer_sizes = [int(x) for x in args.hidden_layer_sizes.split(',')]
   eta_params_base = None
   if args.eta_params is not None:
      eta_params_base = [float(x) for x in args.eta_params.split(',')]
      if len(eta_params_base)!=4:
         error("expecting exactly 4 parameter-specific learning rate modifiers, or None")
   if len(hidden_layer_sizes) < args.n_hidden_layers:
      error("-hidden_layer_sizes list too short")
   hidden_layer_sizes = hidden_layer_sizes[0:args.n_hidden_layers]
   if args.hidden_act == 'tanh':
      hidden_act = T.tanh
   else:
      error("uknown activation function:" + args.hidden_act)
   if args.elide_thist > args.thist_size:
      error("eliding more history (%d) than exists (%d)" % (args.elide_thist, args.thist_size))
   if os.path.exists(args.model):
      error("output file '%s' exists - won't overwrite" % args.model)
   outfile = open(args.model, 'w')

   # Read or generate data

   def checkmax1(v,e): return v[0][0:,:e].max() if v else 0
   def checkmax2(v,b): return v[0][0:,b:].max() if v else 0
   def checkmax3(v): return v[1].max() if v else 0

   if args.train_file:
      log("Reading data")
      D      = read_datafile(args.train_file, args.swin_size, args.thist_size, args.elide_thist)
      D_dev  = read_datafile(args.dev_file, args.swin_size, args.thist_size, args.elide_thist) if args.dev_file else None
      D_test = read_datafile(args.test_file, args.swin_size, args.thist_size, args.elide_thist) if args.test_file else None
      svoc_max = max(D[0][0:,:args.swin_size].max(), checkmax1(D_dev, args.swin_size), checkmax1(D_test, args.swin_size))
      tvoc_max = max(D[0][0:,args.swin_size:].max(), checkmax2(D_dev, args.swin_size), checkmax2(D_test, args.swin_size))
      ovoc_max = max(D[1].max(), checkmax3(D_dev), checkmax3(D_test))
      if svoc_max >= args.svoc_size:
         warn("svoc_size too small (" + str(args.svoc_size) + "); raising it to " + str(svoc_max+1))
         args.svoc_size = svoc_max+1
      if tvoc_max >= args.tvoc_size:
         warn("tvoc_size too small (" + str(args.tvoc_size) + "); raising it to " + str(tvoc_max+1))
         args.tvoc_size = tvoc_max+1
      if ovoc_max >= args.ovoc_size:
         warn("ovoc_size too small (" + str(args.ovoc_size) + "); raising it to " + str(ovoc_max+1))
         args.ovoc_size = ovoc_max+1
   elif args.N:
      log("Generating random data")
      D      = gen_rnd_data(args.N, args.swin_size, args.thist_size, args.svoc_size, args.tvoc_size, args.ovoc_size)
      D_dev  = gen_rnd_data(args.N_dev, args.swin_size, args.thist_size, args.svoc_size, args.tvoc_size, args.ovoc_size)
      D_test = gen_rnd_data(args.N_test, args.swin_size, args.thist_size, args.svoc_size, args.tvoc_size, args.ovoc_size)
   else:
      error("one of -train_file or -N must be specified")

   # Build model (N stands for num examples)

   stvec = T.imatrix("stvec") # src win + tgt hist vectors: N x (swin_size + thist_size)
   ovec  = T.ivector("ovec") # output classes: N x 1
   if args.pretrain_model is not None:
      try:
         with file(args.pretrain_model, 'rb') as f:
            log('Trying to loading symbolic variables from file {}'.format(args.pretrain_model))
            (stvec, ovec, out, sbed, tbed, st_x, hidden_layers) = cPickle.load(f)
            log('Successfully loaded symbolic variables.')

         assert sbed.x_size == args.swin_size * args.embed_size
         assert sbed.vocabularySize() >= args.svoc_size
         assert sbed.embeddingSize() == args.embed_size

         assert tbed.x_size == args.thist_size * args.embed_size
         assert tbed.vocabularySize() >= args.tvoc_size
         assert tbed.embeddingSize() == args.embed_size

         assert len(hidden_layers) == len(hidden_layer_sizes)
         for i in xrange(len(hidden_layer_sizes)):
            assert hidden_layer_sizes[i] == numpy.shape(hidden_layers[i].w.get_value())[1]
      except AssertionError as e:
         error(e)
      except EOFError:
         error('Looks like the pretrain file is incomplete')
      #except Exception as e:
      #   error('Exception while reading symbolic variables from {}'.format(args.pretrain_model))
   else:
      log('Creating new symbolic variables.')
      sbed = embed.EmbedLayer(stvec, args.swin_size, args.svoc_size, args.embed_size, 0, args.embed_bias) # src win embeddings: N x (swin_size*embed_size)
      tbed = embed.EmbedLayer(stvec, args.thist_size, args.tvoc_size, args.embed_size, args.swin_size, args.embed_bias) # tgt hist embeddings: N x (thist_size*embed_size)
      st_x = T.concatenate([sbed.x, tbed.x], 1) # combined embedding
      last_x, last_n = st_x, sbed.x_size+tbed.x_size
      hidden_layers = []
      for hlSize in hidden_layer_sizes:
          hidden_layers.append(
              HiddenLayer(last_x, last_n, hlSize, hidden_act, args.hidden_bias))
          last_x, last_n = hidden_layers[-1].output, hlSize
      out = OutputLayer(last_x, ovec, last_n, args.ovoc_size, args.output_bias)


   # List parameters and define objectives.

   eta_params = [] if eta_params_base is not None else None
   params = sbed.params + tbed.params
   if eta_params_base is not None:
      eta_params.extend([eta_params_base[0] for p in sbed.params])
      eta_params.extend([eta_params_base[1] for p in tbed.params])
   for layer in hidden_layers:
      params += layer.params
      if eta_params_base is not None:
         eta_params.extend([eta_params_base[2] for p in layer.params])
   params += out.params
   if eta_params_base is not None:
      eta_params.extend([eta_params_base[3] for p in out.params])

   L2 = 0
   if args.reg > 0.0:
      for w in params:
         L2 += (w**2).sum()
   LSelf = 0
   if args.self_norm_alpha > 0.0:
      LSelf = (out.norm**2).mean()
   reg_nll = out.nll.mean() + args.reg * L2 + args.self_norm_alpha * LSelf # regularized loss

   # Print status info
   printStatusInfo(args, params, stvec, ovec, sbed, tbed, st_x, hidden_layers, out, D)


   # Train

   data_size = len(D[0])
   nsamples_per_epoch = min(args.batch_size * args.batches_per_epoch, data_size)
   if args.slice_size > nsamples_per_epoch: args.slice_size = nsamples_per_epoch
   nslices_per_epoch = nsamples_per_epoch / args.slice_size
   epoch_beg = data_size
   eta = args.eta_0
   low_val_err = None
   log("Optimizing")
   log("... epoch size = %d (%0.f%% train data), slice size = %d (%d per epoch), minibatch size = %d" \
       % (nsamples_per_epoch, 100.0*nsamples_per_epoch/data_size, args.slice_size, nslices_per_epoch, args.batch_size))
   for epoch in range(args.n_epochs):
      if epoch_beg + nsamples_per_epoch > data_size:
         D_shuff = msgd.shuffle_in_unison(D, rng) # reshuffle data on each pass
         D_shuff = rnd_elide(D_shuff,args.thist_size,args.rnd_elide_max,args.rnd_elide_prob) # optionally elide some target history at random
         epoch_beg = 0
      beg_time = time.clock()
      for slice in range(nslices_per_epoch):
         b = epoch_beg + slice * args.slice_size
         D0 = D_shuff[0][b:b+args.slice_size]
         D1 = D_shuff[1][b:b+args.slice_size]
         err = msgd.optimize(stvec, ovec, reg_nll, params, (D0, D1),
                             valid_set = D_dev if slice+1 == nslices_per_epoch else None,
                             test_set = D_test if slice+1 == nslices_per_epoch else None,
                             error = out.nll.mean(),
                             eta_0 = eta,
                             eta_params = eta_params,
                             n_epochs = 1,
                             batch_size = args.batch_size,
                             val_batch_size = args.val_batch_size,
                             valid_n = args.valid_mean_of_n,
                             update_cap = args.update_cap,
                             shuffle = False,
                             quiet = True)
      log("... epoch %d: %d secs, eta = %f, nlls: train = %f, val = %f, test = %f" \
          % (epoch+1, time.clock()-beg_time, eta, err[0], err[1], err[2]))
      if low_val_err is None or err[1] < low_val_err:
         low_val_err = err[1]
         pending = 0
      else:
         pending += 1
         # eta *= args.decay # Jacob recommends decaying right away, going to try it for now
         if pending == 2:
            eta *= args.decay
         if pending == 4:
            log("validation error stable - quitting")
            break
      epoch_beg += nsamples_per_epoch

   # Write model

   log('Writing to %s' % args.model)
   model = (stvec, ovec, out, sbed, tbed, st_x, hidden_layers)
   cPickle.dump(model, outfile, protocol=cPickle.HIGHEST_PROTOCOL)
   outfile.close()



def main():
   # Arguments

   usage = "train-nnjm.py [options] model"

   help = """
   Train a BBN neural-net joint model on indexed training data in the format
   output by nnjm-genex.py: "sw / h / t", where sw is a window of source words
   centred on the word aligned to t, and h is the ngram preceding t. Write
   the resulting model to file 'model'.

   This program has two modes: use -train_file to train on real data, or -N to
   simulate training with a given number of examples on randomly-generated data.

   If -train_file is specified, the sizes of source, target, and output
   vocabularies will be set to the values of -*voc_size parameters (default values
   if no switches provided) or the maximum values in the train/dev/test files,
   whichever is larger.
   """

   parser = argparse.ArgumentParser(
      formatter_class=argparse.RawDescriptionHelpFormatter,
      usage=usage,
      description=help)
   parser.add_argument('-v', action='count', help='verbose output (repeat for more)', default=0)
   parser.add_argument('-train_file', type=str, default=None, help='file containing training data')
   parser.add_argument('-dev_file', type=str, default=None, help='file containing dev data')
   parser.add_argument('-test_file', type=str, default=None, help='file containing test data')
   parser.add_argument('-N', type=int, default=0, help='number of training examples for random data generation - ignored if -train_file specified')
   parser.add_argument('-N_dev', type=int, default=0, help='number of dev examples for random data generation - ignored if -train_file specified [0]')
   parser.add_argument('-N_test', type=int, default=0, help='number of test examples for random data generation - ignored if -train_file specified [0]')
   parser.add_argument('-swin_size', type=int, default=11, help='number of tokens in source window [11]')
   parser.add_argument('-thist_size', type=int, default=3, help='number of tokens in target history [3]')
   parser.add_argument('-elide_thist', type=int, default=0, help='number of tokens to elide in target history [0]')
   parser.add_argument('-rnd_elide_max', type=int, default=0, help='maximum number of target tokens to randomly elide [0]')
   parser.add_argument('-rnd_elide_prob', type=float, default=1.0, help='probability that a training example will be randomly elided [1.0]')
   parser.add_argument('-svoc_size', type=int, default=0, help='size of source vocab [0]')
   parser.add_argument('-tvoc_size', type=int, default=0, help='size of target vocab [0]')
   parser.add_argument('-ovoc_size', type=int, default=0, help='size of output vocab [0]')
   parser.add_argument('-embed_size', type=int, default=192, help='size of word embeddings [192]')
   parser.add_argument('-embed_bias', type=int, default=0, help='use bias on embedding layer if non-0 [0]')
   parser.add_argument('-n_hidden_layers', type=int, default=2, help='number of hidden layers [2]')
   parser.add_argument('-hidden_layer_sizes', type=str, default='512,512', help='sizes of hidden layers (comma-separated list) [512,512]')
   parser.add_argument('-hidden_act', type=str, default='tanh', help='hidden layer activation function [tanh]')
   parser.add_argument('-hidden_bias', type=int, default=1, help='use bias on hidden layer inputs if non-0 [1]')
   parser.add_argument('-output_bias', type=int, default=1, help='use bias on output layer inputs if non-0 [1]')
   parser.add_argument('-reg', type=float, default=0.0, help='weight on L2 regularization penalty [0.0]')
   parser.add_argument('-eta_0', type=float, default=0.001, help='initial learning rate [0.001]')
   parser.add_argument('-eta_params', type=str, default=None, help='comma-separated list of parameter-specific learning rate modifiers, order is [sbed,tbed,hl,out]')
   parser.add_argument('-n_epochs', type=int, default=10000, help='number of passes through the data [10000]')
   parser.add_argument('-batch_size', type=int, default=128, help='size of mini-batches [128]')
   parser.add_argument('-val_batch_size', type=int, default=50000, help='size of batches for devtest (affects memory use only) [50000]')
   parser.add_argument('-decay', type=float, default=0.5, help='learning rate decay [0.5]')
   parser.add_argument('-print_interval', type=int, default=100, help='interval (# epochs) at which to print learning results [100]')
   parser.add_argument('-slice_size', type=int, default=64000, help='size of training-set slices (affects speed/mem use only) [64000]')
   parser.add_argument('-batches_per_epoch', type=int, default=20000, help='number of mini-batches per epoch (learning rate adjustment point) [20000]')
   parser.add_argument('-self_norm_alpha', type=float, default=0.0, help='weight on the self-normalization term [0.0]')
   parser.add_argument('-update_cap', type=float, default=None, help='maximum absolute weight update per parameter [None]')
   parser.add_argument('-valid_mean_of_n', type=int, default=1, help='validation calculated based on a mean of n scores [1]')
   parser.add_argument('-pretrain_model', type=str, default=None, help='Continue training from an existing model file.')
   parser.add_argument("model", type=str, help="output model file")
   args = parser.parse_args()

   train(args)

if __name__ == '__main__':
   main()
