/**
 * @author Aaron Tikuisis
 * @file ttablewithmax.h  Declaration of TTableWithMax.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "ttable.h"

#ifndef TTABLEWITHMAX_H
#define TTABLEWITHMAX_H

#include "ttable.h"
#include <vector>
#include <string>

using namespace std;

namespace Portage
{
    /// TTable with maximum probability for source/target words distributions.
    class TTableWithMax: public TTable
    {
	private:
            /// Keeps track of the maximum probability for each source.
	    vector<double> maxBySrc;
            /// Keeps track of the maximum probability for each target.
	    vector<double> maxByTgt;
	public:
            /// Construtor.  Constructs a ttable from filename.
            /// @param filename  file containing ttable to load.
	    TTableWithMax(const string &filename);
	    
            /**
             * Get the maximum probability of the source word distribution.
             * @param src_word  source word.
             * @return  Returns the maximum probability of the source word
             * distribution.
             */
	    double maxSourceProb(const string &src_word);
	    
            /**
             * Get the maximum probability of the target word distribution.
             * @param tgt_word  target word.
             * @return  Returns the maximum probability of the target word
             * distribution.
             */
	    double maxTargetProb(const string &tgt_word);
    }; // TTableWithMax
}

#endif // TTABLEWITHMAX_H
