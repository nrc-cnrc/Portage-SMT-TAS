/**
 * @author Eric Joanis
 * @file cube_pruning_decoder.h Decoder that uses Huang and Chiang's (ACL 2007)
 *                              cube pruning method for phrase-based decoding.
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technoogies
 * Institut de technologie de l'information / Institute for Information Technoloy
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __CUBE_PRUNING_DECODER_H__
#define __CUBE_PRUNING_DECODER_H__

#include "canoe_general.h"

namespace Portage {

class BasicModel;
class HypothesisStack;
class CanoeConfig;

/** 
 * Runs the decoder algorithm using Huang and Chiang's cube pruning algorithm.
 *
 * NOTE: Most of paramemeters described below are now members of the
 *       CanoeConfig object, but the documentation is kept here, where
 *       it is relevant.
 *
 * @param model         The model used to score hypotheses
 * @param c.maxStackSize The number of states to keep on each stack (k in Huang
 *                      and Chiang's paper), where stack i contains all
 *                      hypotheses covering i source words.
 *                      This is sometimes called cardinality pruning.
 * @param log(c.pruneThreshold) Relative threshold that further limits stack
 *                      size by rejecting any hypothesis if it is worse that
 *                      the best one by threshold or more.  (Must be a log
 *                      prob, and therefore negative.)
 *                      This is also for cardinality pruning.
 * @param c.covLimit    The maximum number of states to keep with the same
 *                      coverage.  This is sometimes called coverage pruning.
 *                      !!! COVERAGE PRUNING MAY NOT BE IMPLEMENTED YET !!!
 * @param log(c.covThreshold)  Like threshold, but for hypotheses sharing the
 *                      same coverage, rather than on the same stack.  This is
 *                      also for coverage pruning.
 *                      !!! COVERAGE PRUNING MAY NOT BE IMPLEMENTED YET !!!
 * @param c.distLimit   The maximum distortion distance allowed between two
 *                      words.  Use NO_MAX_DISTORTION to mean none.
 * @param c.verbosity   Indicates level of verbosity.
 *
 * @return A pointer to a final hypothesis stack containing the final states.
 *         This stack decoder state must be deleted externally; deleting it
 *         will result in the destruction of the whole lattice.
 */
HypothesisStack* runCubePruningDecoder(BasicModel &model, const CanoeConfig& c);



} // Portage

#endif // __CUBE_PRUNING_DECODER_H__

