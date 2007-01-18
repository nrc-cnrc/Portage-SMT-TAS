/**
 * @author George Foster
 * @file histogram_fit.cc  Histogram data fitting.
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

#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>
#include <cmath>
#include "histogram_fit.h"

using namespace Portage;

void Portage::histoFit(Uint m, double x[], double y[], Uint n, double b[], double v[])
{
   vector< pair<double,double> > xy(m);
   for (Uint i = 0; i < m; ++i)
      xy[i] = make_pair(x[i], y[i]);
   sort (xy.begin(), xy.end());

   double rat = m / double(n+1);

   Uint beg = 0;
   for (Uint i = 0; i < n; ++i) {
      Uint e = (Uint)ceil(rat * (i+1));
      double s = 0.0;
      for (Uint j = beg; j < e; ++j)
         s += xy[j].second;
      b[i] = xy[e].first;
      v[i] = s / (e - beg);
      beg = e;
   }
   double s = 0.0;
   for (Uint j = beg; j < m; ++j)
      s += xy[j].second;
   v[n] = s / (m - beg);

   // average over any consecutive segments having same boundary value

   for (Uint i = 0; i < n; ++i) {
      Uint j;
      for (j = i+1; j < n && b[j] == b[i]; ++j);
      if (j - i - 1) {          // found a run
         if (j == n) ++j;       // include last segment
         double s = accumulate(v+i, v+j, 0.0) / (j-i);
         while (i < j) v[i++] = s;
         --i;                   // gets ++'d by loop
      }
   }
}

double Portage::histoVal(double x, Uint n, const double b[], const double v[])
{
   for (Uint i = 0; i < n; ++i)
      if (x < b[i])
         return v[i];
   return v[n];
}
