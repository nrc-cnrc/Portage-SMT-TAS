#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file embed.py
# @brief Embedding layer to map integer class indexes to a real-valued features.
#
# @author George Foster
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

import sys
import numpy
import numpy.random as rng
import theano
from theano import config, shared, tensor as T

import msgd
import multilayers

class EmbedLayer(object):
    def __init__(self, v, vsize, nclasses, nfeats_per_class, vstart = 0, bias=False):
        """
        Layer to map integer class indexes to a real-valued features.

        Args:
            v: symbolic input matrix, one integer vector per row
            vsize: number of elements to use from each row of v
            nclasses: 1 + max size of elements in v
            nfeats_per_class: number of real-valued features to represent each class
            vstart: index of first element to use in each row of v
            bias: should we add a bias term to each position?
        """
        self.x_size = vsize * nfeats_per_class # size of each output vector
        self.lookup = shared(rng.uniform(-0.05,0.05,size=(nclasses, nfeats_per_class)).astype(config.floatX),
                             name="lookup")
        if bias:
            self.b = shared(numpy.zeros(self.x_size,dtype=theano.config.floatX),name='b')
        # x is lookup[r[0]] ... lookup[r[-1]] for each row r in v:
        # self.x = theano.map(lambda r, lk: T.flatten(lk[r[vstart:vstart+vsize]]), [v], [self.lookup])[0]
        self.x = T.flatten(self.lookup[v[:,vstart:vstart+vsize]], 2)
        if bias: self.x += self.b
        self.params = [self.lookup]
        if bias: self.params.append(self.b)

    def vocabularySize(self):
       return numpy.shape(self.lookup.get_value())[0]

    def embeddingSize(self):
       return numpy.shape(self.lookup.get_value())[1]

    def windowSize(self):
       return self.x_size / self.embeddingSize()



# Test EmbedLayer in a maxent setup.

if __name__ == "__main__":

    rng.seed(1)

    N = 1000 # number of examples
    vsize = 4 # size of class vectors
    nclasses_in = 50 # number of input classes
    nfeats_per_class = 5
    nclasses_out = 8 # number of output classes
    reg = 0.01 # reg wt

    v = T.imatrix("v") # input vectors: N x vsize
    y = T.ivector("y") # output classes: N x 1

    inputs = EmbedLayer(v, vsize, nclasses_in, nfeats_per_class)
    outputs = multilayers.OutputLayer(inputs.x, y, inputs.x_size, nclasses_out)

    reg_nll = outputs.nll.mean() + \
        reg * ((outputs.w**2).sum() + (inputs.lookup**2).sum()) # regularized loss

    D = (numpy.int32(rng.randint(size=(N,vsize), low=0, high=nclasses_in)),
         numpy.int32(rng.randint(size=N, low=0, high=nclasses_out)))

    msgd.optimize(v, y, reg_nll, inputs.params+outputs.params, D, error=outputs.nll, print_interval=100)
