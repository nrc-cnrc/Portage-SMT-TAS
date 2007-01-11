/**
 * @author Aaron Tikuisis
 * @file ttablewithmax.cc  Implementation of TTableWithMax.
 * 
 * 
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#include <iostream>

#include "ttablewithmax.h"

#include <vector>
#include <string>
#include <assert.h>

using namespace std;
using namespace Portage;

TTableWithMax::TTableWithMax(const string &filename): TTable(filename)
{
    maxBySrc.insert(maxBySrc.begin(), numSourceWords(), 0);
    maxByTgt.insert(maxByTgt.begin(), numTargetWords(), 0);
    for (Uint i = 0; i < numSourceWords(); i++)
    {
	for (SrcDistn::const_iterator it = getSourceDistn(i).begin(); it <
		getSourceDistn(i).end(); it++)
	{
	    maxBySrc[i] = max(maxBySrc[i], it->second);
	    maxByTgt[it->first] = max(maxByTgt[it->first], it->second);
	} // for
    } // for
} // TTableWithMax

double TTableWithMax::maxSourceProb(const string &src_word)
{
    assert(numSourceWords() == maxBySrc.size());
    Uint index = sourceIndex(src_word);
    if (index == numSourceWords())
    {
	return 0;
    } else
    {
	return maxBySrc[index];
    } // if
} // maxSourceProb

double TTableWithMax::maxTargetProb(const string &tgt_word)
{
    assert(numTargetWords() == maxByTgt.size());
    Uint index = targetIndex(tgt_word);
    if (index == numTargetWords())
    {
	return 0;
    } else
    {
	return maxByTgt[index];
    } // if
} // maxTargetProb
