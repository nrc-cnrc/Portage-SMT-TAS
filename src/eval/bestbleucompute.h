/**
 * @author Aaron Tikuisis
 * @file bestbleucompute.h  Declaration of the heuristic that finds the best possible score for a set of source and nbest.
 *
 * $Id$
 *
 * Evaluation Module
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * This file contains the implementation of the best BLEU score computation, finding the
 * (probable) best BLEU score obtainable by taking one sentence from each of a set of
 * K-best lists.  It finds the optimum by iteratively improvements, one sentence at a
 * time; this is a heuristic, so it may not be the true optimum, but for reasonable data
 * it's probably correct or at least close.
 */

#ifndef __BEST_BLEU_COMPUTE_H__
#define __BEST_BLEU_COMPUTE_H__

#include <bleu.h>

namespace Portage {
/// Functions to calculate the best BLEU score of a given sources and nbests set.
/// Prevents global namespace pollution in doxygen.
namespace Oracle {

   /**
     * Finds the best BLEU score possible produced by taking one sentence (represented by
     * the sentence's stats) from each vector in [begin, end).  The best score is found by
     * iterative improvement; thus it is a heuristic and may be incorrect.
     * @param begin The beginning of the set of vector<BLEUstats> to use.  Each
     *              vector<BLEUstats> in the range represents possible test sentences
     *              for a fixed source.
     * @param end   The end of the set of vector<BLEUstats> to use.
     * @param K     number of hypotheses for each source.
     * @param bestIndices   A pointer to a vector to use to store the best indices.
     *                      The vector will be resized if it is smaller than the
     *                      range.
     * @param verbose   Whether to produce verbose output.
     * @param bMax      true = use max bleu, false use = min bleu.
     *                  basically finding best oracle score when set to true
     *                  and the worse oracle score when set to false.
     */
    BLEUstats computeBestBLEU(mbIT begin,
        mbIT end, const Uint K, vector<Uint>
        *bestIndices = NULL, bool verbose = false, bool bMax = true);

    /**
     * Finds the best BLEU score possible produced by taking one sentence (represented by
     * the sentence's stats) from each vector in bleu.  The best score is found by
     * iterative improvement; thus it is a heuristic and may be incorrect.
     * @param bleu  A vector of vector<BLEUstats>.  Each vector<BLEUstats> represents
     *              possible test sentences for a fixed source.
     * @param bestIndices   A pointer to a vector to use to store the best indices.
     *                      The vector will be resized if it is smaller than the
     *                      range.
     * @param verbose   Whether to produce verbose output.
     * @param bMax      true = use max bleu, false use = min bleu.
     *                  basically finding best oracle score when set to true
     *                  and the worse oracle score when set to false.
     */
    BLEUstats computeBestBLEU(const MatrixBLEUstats& bleu,
        vector<Uint> *bestIndices = NULL, bool verbose = false, bool bMax = true);
} // ends namespace Oracle
} // ends namespace Portage

#endif //__BEST_BLEU_COMPUTE_H__
