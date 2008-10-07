/**
 * @author Aaron Tikuisis
 * @file perm_iterator.h  Permutation iterator over all permutations of a given length.
 * $Id$
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 * 
 * This file contains the definition of an iterator over all permutations of a given
 * length.
 */

#ifndef PERM_ITERATOR_H
#define PERM_ITERATOR_H

#include "portage_defs.h"
#include <vector>
using namespace std;

namespace Portage
{
    /// An iterator over all permutations of a given length.
    class PermutationIterator
    {
	private:
            /// Initializes the internal values.
	    void init();
            /// Array of size level.
	    vector<Uint> valsV;
	    
	    /**
	     * Advances to the next permutation.  See inline documentation for a more
	     * precise explanation.
             * @param level
             * @return 
	     */
	    bool doStep(Uint level);
	public:
	    /**
	     * Creates a new PermutationIterator for permutations on {0, ... , length-1}.
	     * The first permutation is available in [vals, .. , valsEnd).
	     * @param length	The length of permutations to iterate over.
	     */
	    PermutationIterator(Uint length);
	    
	    /**
	     * Provides public access to the current permutation.  Explicitly, the
	     * permutation is:
	     * (   0        1           length-1  )
	     * ( vals[0]  vals[1]  ..  valsEnd[-1])
	     */
	    const vector<Uint>::const_iterator vals;
            /// See vals
	    const vector<Uint>::const_iterator valsEnd;
	    
	    /**
	     * Steps to the next permutation, returning true iff it is not the same as the
	     * first permutation.  Note that after the iterator has come to the first
	     * permutation, step() may still be called and the iteration restarts from the
	     * beginning.
	     * Amortized O(length) cost.
             * @return Returns iff it is not the same as the first permutation.
	     */
	    bool step();
    }; // PermutationIterator
} // Portage

#endif // PERM_ITERATOR_H
