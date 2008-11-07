/**
 * @author George Foster, Eric Joanis
 * @file gfmath.h Simple math stuff.
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

#ifndef GFMATH_H
#define GFMATH_H

#include <cmath>
#include <vector>
#include "portage_defs.h"

namespace Portage {

#ifndef log2
extern double log2(double x);
#endif
extern double exp2(double x);

/**
 * Round up to the smallest power of 2 >= x
 */
extern Uint next_power_of_2(Uint x);

/**
 * Fast calculation for log(exp(logx) + exp(logy)).
 * Thanks to Howard Johnson.
 * Note: fast with two numbers, but not optimal when used to sum a vector of
 * logs.
 */
template<class T> T logsum(double logx, double logy)
{
   if ( logy > logx ) swap(logx, logy);
   T del = logx - logy;
   if ( del > 35.0 ) return logx;
   T eps = exp( -del );
   if ( del < 12.0 ) return logx + log( 1.0 + eps );
   return logx + eps - eps * eps / 2.0;
}

/**
 * Fast calculation for log sum_i (exp(log_i)), based on 2-argument version of
 * logsum. 
 * Warning: this is not as fast as hoped.
 */
template<class T> T logsum(vector<T>& v)
{
   sort(v.begin(), v.end());
   T sum = v.size() ? v[0] : 0.0;
   for (Uint i = 1; i < v.size(); ++i)
      sum = logsum<T>(sum, v[i]);
   return sum;
}

} // Portage
#endif
