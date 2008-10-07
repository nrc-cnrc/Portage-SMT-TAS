/**
 * @author Nicola Ueffing
 * @file confidence_score.h  Class for storing word confidence measures.
 *
 * $Id$ 
 *
 *
 * COMMENTS: class for storing word confidence measures calculated over N-best lists
 * contains word posterior probability, rank weighted frequency, and relative frequency
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef CONFIDENCE_SCORE_H
#define CONFIDENCE_SCORE_H

#include <iostream>
#include "portage_defs.h"

//typedef unsigned int Uint;

namespace Portage {

    /// Placeholder for storing word confidence measures.
    class ConfScore {
    private:
	double wpp, rankf, relf;
	
    public:
        /// Empty constructor.
	ConfScore() : wpp(INFINITY), rankf(0), relf(0) {}
        /**
         * Constructor.
         * @param p
         * @param n
         * @param f
         */
	ConfScore(double p, Uint n, Uint f) : wpp(p), rankf(n), relf(f) {}
        /// Copy constructor.
        /// @param c  operand to copy.
	ConfScore(const ConfScore &c) : wpp(c.wpp), rankf(c.rankf), relf(c.relf) {}
        /// Destructor.
	~ConfScore() {};
	
        /// Set all values back to default.
	void   reset();

        /**
         * Update confidence score: add (log) probability, rank, increase frequency.
         * @param p
         * @param n
         */
	void   update(double p, Uint n);
        /**
         * Add other confidence score to this.
         * @param conf  operand to update from
         */
	void   update(const ConfScore &conf);

        /**
         * Normalize probability by given value t, and rank sum and  frequency by given N-best list length N.
         * @param t
         * @param N
         */
	void   normalize(double t, Uint N);
        /**
         * Normalize probability by given confidence score which represents total probability mass etc.
         * @param c
         */
	void   normalize(const ConfScore &c);

        /// Get the value of probability.
        /// @return Returns the probability.
	double prob() const;
        /// Get the value of rank sum.
        /// @return Returns the rank.
	double rank() const;
        /// Get the relative frequency.
        /// @return Returns the relative frequency.
	double relfreq() const;
	
    };
}

#endif
