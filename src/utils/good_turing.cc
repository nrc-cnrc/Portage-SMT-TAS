/**
 * @author George Foster
 * @file good_turing.cc  Good-Turing frequency smoothing.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include "errors.h"
#include "good_turing.h"

using namespace std;
using namespace Portage;

GoodTuring::GoodTuring(Uint n, const Uint item_freqs[]) 
{
   map<Uint,Uint> count_map;
   for (Uint i = 0; i < n; ++i) {count_map[item_freqs[i]]++;}

   vector<Uint> freqs, freq_counts;
   for (map<Uint,Uint>::const_iterator p = count_map.begin(); p != count_map.end(); ++p) {
      freqs.push_back(p->first);
      freq_counts.push_back(p->second);
   }

   constructFromCounts(freqs.size(), &freqs[0], &freq_counts[0]);
}


void GoodTuring::constructFromCounts(Uint n, const Uint freqs[], const Uint freq_counts[])
{
   vector<double> logfreqs(n);
   vector<double> logcounts(n);
   vector<double> weights(n);
   double max_count = *max_element(freq_counts, freq_counts+n);

   for (Uint i = 0; i < n; ++i) {
      logfreqs[i] = log((double)freqs[i]);
      logcounts[i] = log((double)freq_counts[i]);
      weights[i] = freq_counts[i] / max_count;
   }
   
   poly_coeffs.resize(poly_order+1);
   if (!LSPolyFit(n, &logfreqs[0], &logcounts[0], poly_order, &poly_coeffs[0], &weights[0]))
      error(ETFatal, "GoodTuring: couldn't fit least-squares line");
}
