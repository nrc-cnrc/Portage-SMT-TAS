#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# @file multilayers.py
# @brief Multi-layer perceptron
# @author Marine Carpuat
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

import numpy
import numpy.random as rng
import theano
import theano.tensor as T
import msgd

class HiddenLayer(object):
    def __init__(self, x, n_in, n_out, activation=T.tanh, bias=False):
        """
        Hidden layer class
        :type x: T.TensorType
        :param x: symbolic variable that describes inputs

        :type n_in: int
        :param n_in: input space dimension

        :type n_out: int
        :param n_out: output space dimension

        :type activation: theano.op or function
        :param activation: non linearity to be applied in the hidden layer

        :type bias Boolean
        :param bias include bias term if true
        """
        # weight matrix W is of shape n_in x n_out
        # initialize weights w uniformly in interval that depends on the activation function, as recommended in [Xavier10]
        # http://deeplearning.net/tutorial/mlp.html#mlp
        w_values = numpy.asarray(rng.uniform(
                low=-numpy.sqrt(6. / (n_in + n_out)),
                high=numpy.sqrt(6. / (n_in + n_out)),
                size=(n_in,n_out)), dtype = theano.config.floatX)
        if activation == T.nnet.sigmoid:
            w_values *= 4
        self.w = theano.shared(value=w_values, name='w')
        self.b = theano.shared(numpy.zeros(n_out, dtype=theano.config.floatX), name='b')
        self.output = activation(T.dot(x,self.w) + self.b)
        self.params = [self.w]
        if bias: self.params.append(self.b)

class OutputLayer(object):
    def __init__(self, x, y, n_in, n_out, bias=False):
       """
       Output layer class
       :type x: T.TensorType
       :param x: symbolic variable that describes inputs

       :type y: T.TensorTYpe
       :param y: symbolic variable that describes outputs

       :type n_in: int
       :param n_in: input space dimension

       :type n_out: int
       :param n_out: output space dimension

       :type bias: bool
       :param bias: include bias term if true
       """
       self.w = theano.shared(rng.uniform(-0.05,0.05, (n_in,n_out)).astype(theano.config.floatX), name="w") # weights: number of features n_in x number of classes n_out
       init_b = numpy.empty(n_out)
       if bias:
           init_b.fill(numpy.log(1.0/n_out)) # Output weights initially log(sum(exp))=0
       else:
           init_b.fill(0.0)
       self.b               = theano.shared(init_b.astype(theano.config.floatX), name = 'b')
       self.s               = T.dot(x, self.w)+self.b
       sMax                 = T.max(self.s, axis = 1, keepdims = True)
       self.norm            = T.log(T.sum(T.exp(self.s-sMax), axis = 1, keepdims = True)) + sMax # expose the norm for self normalization
       self.log_p_y_given_x = self.s - self.norm
       logp                 = self.log_p_y_given_x[T.arange(y.shape[0]), y] # probs for correct label
       self.nll             = -logp # negative log likelihood
       self.params          = [self.w]
       if bias: self.params.append(self.b)

class SingleHiddenLayerPerceptron(object):
    def __init__(self, x, y, n_in, n_out, n_hidden):
        self.hiddenLayer = HiddenLayer(x, n_in=n_in, n_out=n_hidden,
                                       activation=T.tanh)
        self.outputLayer = OutputLayer(x=self.hiddenLayer.output, y=y,
                                       n_in=n_hidden, n_out = n_out)
        self.nll = self.outputLayer.nll
        self.L2squared = (self.hiddenLayer.w ** 2).sum() + (self.outputLayer.w ** 2).sum()
        self.params = [self.hiddenLayer.w] + [self.outputLayer.w]

class MultiLayerPerceptron(object):
    def __init__(self, x, y, n_in, n_out, hidden_layer_sizes = [10, 10]):
        self.n_layers = len(hidden_layer_sizes)
        assert self.n_layers > 0

        # construct hidden layers - I am arbitrarily forcing all layers to use a tanh activation function for now
        self.hiddenLayers = []
        for i in xrange(self.n_layers):
            if i == 0:
                layer_n_in = n_in
                layer_x = x
            else:
                layer_n_in = hidden_layer_sizes[i - 1]
                layer_x = self.hiddenLayers[i-1].output
            hl = HiddenLayer(x = layer_x,
                             n_in=layer_n_in,
                             n_out=hidden_layer_sizes[i],
                             activation=T.tanh)
            self.hiddenLayers.append(hl)
        # add logistic regression on top
        self.outputLayer = OutputLayer(x=self.hiddenLayers[self.n_layers-1].output, y=y,
                                       n_in=hidden_layer_sizes[self.n_layers-1], n_out = n_out)
        self.nll = self.outputLayer.nll

        # store all matrix weights w for all layers
        self.params = []
        for i in xrange(self.n_layers):
            self.params += [self.hiddenLayers[i].w]
        self.params += [self.outputLayer.w]

    def L2squared(self):
        l2sq = (self.outputLayer.w ** 2).sum()
        for i in xrange(self.n_layers):
            l2sq += (self.hiddenLayers[i].w ** 2).sum()
        return l2sq


def mlp(train_set, reg = 0.01, hidden_layer_sizes = [10, 10], **optimize_kwargs):
    assert(train_set and len(train_set) and len(train_set[0]))
    nfeats = len(train_set[0][0])
    nclasses = max(train_set[1])+1

    x = T.matrix("x")  # feature vectors: N x nfeats
    y = T.ivector("y")  # labels: N x 1
    classifier = MultiLayerPerceptron(x, y, nfeats, nclasses, hidden_layer_sizes)
    reg_nll = classifier.nll.mean()
    reg_nll += reg * classifier.L2squared() # regularized loss
    msgd.optimize(x, y, reg_nll, classifier.params, train_set, error=classifier.nll, **optimize_kwargs)
    return classifier.params


def onelayer(train_set, reg = 0.01, layer_size = 20, **optimize_kwargs):
    assert(train_set and len(train_set) and len(train_set[0]))
    nfeats = len(train_set[0][0])
    nclasses = max(train_set[1])+1

    x = T.matrix("x")  # feature vectors: N x nfeats
    y = T.ivector("y")  # labels: N x 1
    classifier = SingleHiddenLayerPerceptron(x,y, nfeats, nclasses, layer_size)
    reg_nll = classifier.nll.mean() + reg * classifier.L2squared # regularized loss
    msgd.optimize(x, y, reg_nll, classifier.params, train_set, error=classifier.nll, **optimize_kwargs)
    return classifier.params


def maxent(train_set, reg = 0.01, **optimize_kwargs):
    """Train a maxent model on given data, using Theano.

    This is a simple wrapper for optimize.msgd() that defines the maxent loss
    and provides an interface free of Theano symbolic variables. This model
    doesn't include an explicit bias, so you need to include a feature whose
    value is constant if you want bias.

    Args:
       train_set: (feat_vects, labels), where feat_vects is a N x nfeats array
	  of feature vectors, and labels is an N x 1 array of integer label
          indexes; assumed to be non-empty
       reg: weight on L2 regularization penalty
       **optimize_kwargs: list of key=val pairs for any optional arguments to
          optimize.msgd() except 'error'
    Returns:
       nclasses x nfeats array of trained weights, where nclasses is determined
          based on the highest label index in train_set.
    """
    assert(train_set and len(train_set) and len(train_set[0]))
    nfeats = len(train_set[0][0])
    nclasses = max(train_set[1])+1

    x = T.matrix("x")  # feature vectors: N x nfeats
    y = T.ivector("y")  # labels: N x 1
    classifier = OutputLayer(x,y, nfeats, nclasses)
    reg_nll = classifier.nll.mean() + reg * (classifier.w**2).sum() # regularized loss
    msgd.optimize(x, y, reg_nll, classifier.params, train_set, error=classifier.nll, **optimize_kwargs)
    return classifier.params


if __name__=='__main__':
   rng.seed(1)
   N = 10   # num examples
   nfeats = 5 # num features
   nclasses = 3 #  num classes

   # training data: N x nfeats vectors, with [0,N) output:
   D = (rng.randn(N, nfeats), numpy.int32(rng.randint(size=N, low=0, high=nclasses)))
   print "Training maxent model"
   w = maxent(D, print_interval=100)
   print "Final weights (one column / class):",'\n', w[0].get_value()
   print "Training perceptron with 1 hidden layer, layer size = 10"
   w = onelayer(D, reg = 0.01, layer_size = 10, print_interval=100)
   print "Hidden layer weights (one column / hidden unit):",'\n', w[0].get_value()
   print "Output layer weights (one column / class):",'\n', w[1].get_value()
   sizes = [5, 5, 5]
   print "Training multilayer perceptron, layer sizes = ", sizes
   w = mlp(D, reg = 0.01, hidden_layer_sizes = sizes, print_interval=100)
   for i in xrange(len(sizes)):
       print "Weights for hidden layer ", i ," (one column / hidden unit):",'\n', w[i].get_value()
   print "Output layer weights (one column / class):",'\n', w[len(sizes)].get_value()
