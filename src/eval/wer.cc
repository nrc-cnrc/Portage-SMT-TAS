/**
 * @author George Foster (with comments by Aaron Tikuisis)
 * @file wer.cc  Implementation of functions that calculates WER and PER.
 *
 * $Id$
 * 
 * Evaluation Module
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada 
 * 
 * This file contains the implementation of mWER computation.
 */

#include "wer.h"
#include <portage_defs.h>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

namespace Portage
{
    /// Element in WER matrix.
    struct WerElement		
    {
	Uint cost;		///< cost of best path.
	Uint len;		///< length of best path.
        /**
         * Constructor.
         * @param cost
         * @param len
         */
	WerElement(Uint cost, Uint len) : cost(cost), len(len) {}
        /**
         * Constructor that makes WerElement with cost = 0 and len = 0
         */
	WerElement() {cost = len = 0;}
    }; // WerElement
    
    Uint find_mWER(const vector<string>& tst, const vector<string>& ref, Uint* len_of_best)
    {
	Uint n = tst.size();
	Uint m = ref.size();
	
	WerElement *wer_matrix[n+1];
	for (Uint i = 0; i <= n; ++i)
	{
	    wer_matrix[i] = new WerElement[m+1];
	} // for
	
	for (Uint i = 0; i <= n; ++i) wer_matrix[i][0] = WerElement(i,i);
	for (Uint j = 0; j <= m; ++j) wer_matrix[0][j] = WerElement(j,j);
	
	for (Uint i = 1; i <= n; ++i)
	{
	    for (Uint j = 1; j <= m; ++j)
	    {
		Uint above = wer_matrix[i-1][j].cost + 1;	// Insertion
		Uint left = wer_matrix[i][j-1].cost + 1;	// Removal
		Uint best = wer_matrix[i-1][j-1].cost + (tst[i-1] == ref[j-1] ? 0 : 1);
		// Match or substitution
		
		Uint len = wer_matrix[i-1][j-1].len + 1;
		if (left < best)
		{
		    best = left;
		    len = wer_matrix[i][j-1].len + 1;
		} // if
		if (above < best)
		{
		    best = above;
		    len = wer_matrix[i-1][j].len + 1;
		} // if	 
		wer_matrix[i][j] = WerElement(best, len);
	    } // for
	} // for
	WerElement result = wer_matrix[n][m];
	for (Uint i = 0; i <= n; ++i)
	{
	    delete [] wer_matrix[i];
	} // for
	if (len_of_best)
	{
	    *len_of_best = result.len;
	} // if
	return result.cost;
    } // find_mWER
    
    Uint find_mPER(vector<string> tst, vector<string> ref)
    {
	sort(tst.begin(), tst.end());
	sort(ref.begin(), ref.end());
	vector<string>::const_iterator tstIt = tst.begin();
	vector<string>::const_iterator refIt = ref.begin();
	Uint numMatches = 0;
	while (tstIt != tst.end() && refIt != ref.end())
	{
	    if (*tstIt == *refIt)
	    {
		numMatches++;
		tstIt++;
		refIt++;
	    } else if (*tstIt < *refIt)
	    {
		tstIt++;
	    } else // (*tstIt > *refIt)
	    {
		refIt++;
	    } // if
	} // while
	return max(tst.size(), ref.size()) - numMatches;
    } // find_mPER
    
} // Portage
