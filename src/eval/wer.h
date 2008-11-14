/**
 * @author George Foster (with comments by Aaron Tikuisis)
 * @file wer.h  Declaration of functions that calculates WER and PER.
 *
 * $Id$
 * 
 * Evaluation Module
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 * 
 * This file contains the declaration of a function used to compute mWER (minimal word
 * error-rate) and mPER (minimal position-independant word error-rate).
 */

#ifndef WER_H
#define WER_H
#include <vector>
#include <portage_defs.h>
#include <string>

using namespace std;

namespace Portage
{
    /**
     * Calculates WER.
     * @param tst          source sentence
     * @param ref          reference sentence
     * @param len_of_best  if not null return the length of the best path.
     * @return Returns the number of insertions, removals and substitutions
     * required to modify tst into ref.
     */
    Uint find_mWER(const vector<string> &tst, const vector<string>& ref, Uint*
	    len_of_best = NULL);
    
    /**
     * Calculates PER.
     * @param tst  source sentence
     * @param ref  reference sentence
     * @return Returns the number of changes required to transform tst in to ref.
     */
    Uint find_mPER(vector<string> tst, vector<string> ref);
} // Portage

#endif // WER_H
