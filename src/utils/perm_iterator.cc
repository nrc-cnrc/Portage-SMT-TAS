/**
 * @author Aaron Tikuisis
 * @file perm_iterator.cc  Permutation iterator over all permutations of a given length.
 * $Id$
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 * 
 * This file contains an implementation of an iterator over all permutations of a given
 * length.
 */

#include "portage_defs.h"
#include "perm_iterator.h"
#include <vector>

using namespace std;
using namespace Portage;

PermutationIterator::PermutationIterator(Uint size): valsV(size), vals(valsV.begin()),
    valsEnd(valsV.end())
{
    init();
} // PermutationIterator()

void PermutationIterator::init()
{
    for (Uint i = 0; i < valsV.size(); i++)
    {
	valsV[i] = i;
    } // for
} // init()

bool PermutationIterator::step()
{
    return doStep(valsV.size() - 1);
} // step()

bool PermutationIterator::doStep(Uint level)
{
    /*
     * Consider a "permutation tree": that is, a decision tree for creating a permutation
     * by picking each successive value.  eg. for length L=3 and calling our permutation
     * \phi:
     *
     *             (root)
     *            /   |   \
     * \phi(0)   0    1    2
     *           /\   /\   /\
     * \phi(1)  1  2 0  2 0  1
     *          |  | |  | |  |
     * \phi(2)  2  1 2  0 1  0
     *
     * We can create such a planar tree uniquely by ordering all sibling nodes in
     * ascending order, as is done in the example.  We iterate through all permutations by
     * doing a iterating through the leaves of this tree from left to right.  More
     * precisely, we perform a left-to-right, pre-order traversal of the entire tree, but
     * only break at the leaves.
     * Every partial permutation (ie. a (1-1) function \phi:{0,..,K-1} -> {0,..,L-1} where
     * K<L) specifies a node in this tree.  doStep() takes a node specified in this manner
     * and continues the pre-order traversal assuming that this node has just been
     * visited.  Via recursion, it stops only when a leaf node is found.
     * The partial permutation is given in valsV[0..level], and
     * valsV[level+1..valsV.size()-1] is required to be in ascending order.
     * 
     * Complete preconditions:
     * - level < valsV.size(),
     * - valsV[level + 1],  ... , valsV[valsV.size() - 1] is in ascending order,
     * - valsV is a permutation.
     * 
     * Runtime cost considerations:
     * - At depth D (where the root is at depth 0), there are L! / (L - D)! nodes.
     * - For D < L, visiting the node at depth D is \Omega(L-D) and finding the next node
     *   to visit is \Omega(L-D).  (note: these two steps are done in the reverse order
     *   here, but this order is more convenient or else D changes ).
     * - Visiting nodes at depth L and finding the next node is \Omega(1).
     * - So, total for traversing the entire tree is
     *     \Omega( \sum{0 <= D <= L} (L-D) * L!/(L-D)! )
     *   = \Omega( \sum(0 <= D <= L) L!/(L-D-1)! )
     *   = O((L+1)!) (I can't figure it out exactly, but it's probably close to O(L))
     * - What we are really interested in is amortized cost for step().  Since this is
     *   called L! times for an entire traversal of the tree, step() is O(L).
     */
    
    Uint tmpLevel;
    
    // We set tmpLevel to the first index after level s.t vals[level] < vals[tmpLevel] to
    // determine which node to visit
    
    for (tmpLevel = level + 1; tmpLevel < valsV.size() && vals[level] > vals[tmpLevel];
	    tmpLevel++) {}
    
    // Next we visit that node
    if (tmpLevel == valsV.size())
    {
	// If vals[l] > vals[level] for all l > level, then this is the final sibling to
	// visit at level.
	if (level == 0)
	{
	    // The next node in our traversal is the root node.  Hence, there are no more
	    // leaves to visit so we restart.
	    init();
	    return false;
	} else
	{
	    // Continue the traversal by visiting the parent node next.  This requires
	    // that vals[level], .. , vals[vals.size() - 1] be put in ascending order.
	    // Since vals[level + 1], .. , vals[vals.size() - 1] are in order and
	    // vals[level] > vals[vals.size() - 1], all we need to do is remove
	    // vals[level] and insert it and the end.
	    Uint tmp = vals[level];
	    for (tmpLevel = level; tmpLevel < valsV.size() - 1; tmpLevel++)
	    {
		valsV[tmpLevel] = vals[tmpLevel + 1];
	    }
	    valsV[valsV.size() - 1] = tmp;
	    
	    return doStep(level - 1);
	} // if
    } // if
    
    // The next node to visit is a leaf, and is obtained by swapping vals[tmpLevel] with
    // vals[level].
    Uint tmp = vals[tmpLevel];
    valsV[tmpLevel] = vals[level];
    valsV[level] = tmp;
    return true;
} // doStep()
