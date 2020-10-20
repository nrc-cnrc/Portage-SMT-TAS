#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# @file msgd.py
# @brief Mini-batch SGD for Theano.
#
# @author Colin Cherry
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

import time
import sys
import os

import math
import numpy
import theano
import theano.tensor as T

"""Mini-batch SGD for Theano

Idea: provide only theao formulas and variables, SGD will handle the rest.

:type x: theano.tensor.TensorType
:param x: symbolic variable that describes the input to the model

:type y: theano.tensor.TensorType
:param y: symbolic variable that describes the model output;
used only to represent the gold standard in any formulas

:type cost: theano.tensor.TensorVariable
:param cost: symbolic variable representing the cost or loss function of
             the learner; will usually reference x and y

:type params: list theano.tensor.SharedVariable
:param params: list of shared variables representing model parameters

:type train_set: (data_x, data_y) where data_x can feed a function expecting x,
                 and data_y can feed a function expeting y
:param train_set: (data_x, data_y) pair to be used in training

:type valid_set: (data_x, data_y)
:param valid_set: Validation set to determine early stopping

:type test_set: (data_x, data_y)
:param test_set: Test set to be evaluated at each potentially best model

:type error: theano.tensor.TensorVariable
:param error: symbolic variable representing the error function for the learner
              if this is missing, cost will be used in its place

:type eta_0: double
:param eta_0: Initial learning rate

:type eta_params: list double
:param eta_params: list of parameter-specific learning rate modifiers

:type n_epochs: int
:param n_epochs: Maximum number of passes through the training set

:type batch_size: int
:param batch_size: Size of each mini-batch, will truncate to size of training set

:type val_batch_size: int
:param val_batch_size: Size of batches in which to divide val and test sets when
   calculating error, to avoid memory overflow; this does not affect results

:type decay: double (0<=decay<=1)
:param decay: learning rate decay is raised to this exponent: 1=fastest, 0=no decay

:type epsilon: double
:param epsilon: scale-free sensitivity to model improvements for early stopping

:type print_interval: int
:param print_interval: print status every x epochs

:type shuffle: bool
:param shuffle: shuffle data before training

:type quiet: bool
:param quiet: no noise, even at end of training

"""

def optimize(
    x,
    y,
    cost,
    params,
    train_set,
    valid_set=None,
    test_set=None,
    error=None,
    eta_0=1,
    eta_params=None,
    n_epochs=10000,
    batch_size=100,
    val_batch_size=50000,
    valid_n=5,
    decay=1,
    epsilon=1e-6,
    print_interval=1000,
    update_cap=None,
    shuffle=True,
    quiet=False):

    """
    Encapsulate mini-batch SGD into a function
    """

    num_samples = train_set[0].shape[0]
    if batch_size > num_samples:
        print >> sys.stderr, "truncating batch size to data size"
        batch_size = num_samples

    if decay < 0 or decay > 1:
        print >> sys.stderr, "invalid decay:",decay
        sys.quit()

    if eta_params is None:
        eta_params = [1. for p in params]
    if len(eta_params)!=len(params):
        print sys.stderr, "invalid number of parameter-specific learning rates: ",len(eta_params)
        sys.quit()

    ################
    # Prepare Data #
    ################

    if not quiet: print >> sys.stderr, "... moving data to shared memory"

    # compute number of minibatches for training
    n_train_batches = int(math.ceil(num_samples / float(batch_size)))

    rng = numpy.random
    perm_train_set = shuffle_in_unison(train_set,rng) if shuffle else train_set
    train_set_x, train_set_y = shared_dataset(x, y, perm_train_set)

    # if error is missing, then use cost
    if error is None:
        error = cost

    # compiling a Theano function that computes the mistakes that are made by
    # the model on a test or validation set

    vb_beg_index = T.lscalar('vb_beg_index')  # start index to a devtest batch
    vb_end_index = T.lscalar('vb_end_index')  # end index to a devtest batch

    if test_set is not None:
        test_set_x, test_set_y = shared_dataset(x, y, test_set)
        test_model = theano.function(
            inputs=[vb_beg_index, vb_end_index],
            outputs=error,
            givens={
                x: test_set_x[vb_beg_index:vb_end_index],
                y: test_set_y[vb_beg_index:vb_end_index]})

    if valid_set is not None:
        valid_set_x, valid_set_y = shared_dataset(x, y, valid_set)
        validate_model = theano.function(
            inputs=[vb_beg_index, vb_end_index],
            outputs=error,
            givens={
                x: valid_set_x[vb_beg_index:vb_end_index],
                y: valid_set_y[vb_beg_index:vb_end_index]})

    ###############
    # Build Model #
    ###############

    if not quiet: print >> sys.stderr, '... building the model'

    index = T.lscalar('index')               # index to a [mini]batch
    itern = theano.shared(numpy.asarray(0., dtype=theano.config.floatX), name='itern')
    eta = eta_0 / ((1. + eta_0*itern)**decay) # learning rate

    # specify how to update the parameters of the model as a list of
    # (variable, update expression) pairs.
    if update_cap is not None:
        updates = [ (p, p - T.minimum(update_cap, T.maximum(-update_cap, eta * ep * T.grad(cost, p))))
                    for p, ep in zip(params, eta_params) ]
    else:
        updates = [ (p, p - eta * ep * T.grad(cost, p))
                    for p, ep in zip(params, eta_params) ]
    # Not sure automatic learning rate reductions are the way to go, especially for MNIST
    # updates.append( (itern, itern+1) )

    # Function to trigger learning rate reduction on a failure to improve
    update_eta = theano.function(inputs=[], outputs=[itern], updates=[(itern, itern+1)])

    # compiling a Theano function `train_model` that returns the cost, but in
    # the same time updates the parameter of the model based on the rules
    # defined in `updates`
    train_model = theano.function(
        inputs=[index],
        outputs=[cost,eta],
        updates=updates,
        givens={
            x: train_set_x[index * batch_size:(index + 1) * batch_size],
            y: train_set_y[index * batch_size:(index + 1) * batch_size]})

    ###############
    # TRAIN MODEL #
    ###############
    if not quiet: print >> sys.stderr, '... training the model'
    best = None
    bestLoss = None
    willingToWait=10
    start_time = time.clock()

    done_looping = False
    epoch = 0
    while (epoch < n_epochs) and (not done_looping):
        epoch = epoch + 1
        loss = 0
        if shuffle:
           g = rng.permutation(xrange(n_train_batches))
        else:
           g = xrange(n_train_batches)

        iter = 0
        valid_mean = 0
        valid_i = 0
        valid_check = n_train_batches - calc_valid_check(valid_i, valid_n)
        for minibatch_index in g:
            minibatch_avg_cost, eta = train_model(minibatch_index)
            loss = loss + minibatch_avg_cost
            # iteration number
            iter = iter + 1
            # Following Jacob's README, get the mean of 5 validation checks
            if valid_set is not None and iter == valid_check:
                valid_i = valid_i + 1
                valid = calc_error_by_batches(validate_model, valid_set[0].shape[0],
                                              val_batch_size)
                valid_mean = valid_mean + (valid - valid_mean) / valid_i
                valid_check = n_train_batches - calc_valid_check(valid_i, valid_n)
        loss = loss / n_train_batches

        # Check to reduce learning rate
        if (bestLoss is None) or ( (bestLoss - loss) / abs(bestLoss) > epsilon):
            bestLoss = loss
        else:
            update_eta()

        # Calculate stopping criterion
        if valid_set is not None:
            obj = valid_mean
        else:
            obj = loss

        # Check stopping criterion
        if (best is None) or ( (best - obj) / abs(best) > epsilon):
            best = obj
            finalLoss = loss
            if valid_set is not None:
                best_validation_loss = obj
            if test_set is not None:
                test_score = calc_error_by_batches(test_model, test_set[0].shape[0],
                                                   val_batch_size)
            waitingForBest=0
        elif waitingForBest<willingToWait:
            waitingForBest=waitingForBest+1
        else:
            break

        if epoch%print_interval==0:
            print >> sys.stderr, ('{:4d}'.format(epoch) +
            ' %0.4f'%(eta) +
            ' %f'%(loss) +
            '{valid}'.format(valid=' %f'%(obj) if valid_set is not None else '' ))

    end_time = time.clock()
    if not quiet:
        print >> sys.stderr, ('Optimization complete with final loss of %f' % (loss))
        if valid_set is not None:
            print >> sys.stderr, ('Best validation score is: %f' % (best_validation_loss))
        if test_set is not None:
            print >> sys.stderr, ('Test score of final model is %f' % (test_score ))
        print >> sys.stderr, 'Ran for %d epochs, with %f epochs/sec' % (
            epoch, 1. * epoch / (end_time - start_time))
        print >> sys.stderr, ('The code for file ' +
                             os.path.split(__file__)[1] +
                             ' ran for %.1fs' % ((end_time - start_time)))

    return loss, \
           best_validation_loss if valid_set else 0, \
           test_score if test_set else 0

def shared_dataset(x, y, data_xy, borrow=True):
    """ Function that loads the dataset into shared variables

    The reason we store our dataset in shared variables is to allow
    Theano to copy it into the GPU memory (when code is run on GPU).
    Since copying data into the GPU is slow, copying a minibatch everytime
    is needed (the default behaviour if the data is not in a shared
    variable) would lead to a large decrease in performance.
    """
    data_x, data_y = data_xy
    shared_x = theano.shared(numpy.asarray(data_x,
                                           dtype=theano.config.floatX),
                             borrow=borrow)
    shared_y = theano.shared(numpy.asarray(data_y,
                                           dtype=theano.config.floatX),
                             borrow=borrow)
    # When storing data on the GPU it has to be stored as floats
    # therefore we will store the labels as ``floatX`` as well
    # (``shared_y`` does exactly that). But during our computations
    # we need them as ints (we use labels as index, and if they are
    # floats it doesn't make sense) therefore instead of returning
    # ``shared_y`` we will have to cast it to int. This little hack
    # lets ous get around this issue
    return T.cast(shared_x, x.dtype), T.cast(shared_y, y.dtype)

def shuffle_in_unison(data_xy, rng):
    data_x, data_y = data_xy
    assert len(data_x)==len(data_y), "%s!=%s"%(len(data_x),len(data_y))
    p = rng.permutation(len(data_x))
    return [data_x[p], data_y[p]]

def calc_error_by_batches(model, data_size, batch_size):
    """Calculate error in batches using the given valid/test model."""
    err = 0.0
    beg = 0
    while (beg < data_size):
        end = min(beg+batch_size, data_size)
        err += model(beg,end) * (end-beg)
        beg = end
    return err / data_size

def calc_valid_check(i, n):
    """Calculate the iteration number of the i'th validation check out of n checks."""
    if n==1: return 0
    else: return ((n-1)-i)*(100/(n-1))
