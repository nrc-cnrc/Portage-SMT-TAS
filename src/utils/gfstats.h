/**
 * @author George Foster
 * @file gfstats.h Simple stats algorithms.
 *
 *
 * COMMENTS:
 *
 * Template-haters beware.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef GFSTATS_H
#define GFSTATS_H

#include <cmath>
#include <vector>
#include "portage_defs.h"
#include <cstdlib>

namespace Portage {

/**
 * Compute mean.
 * @param sum  sum of items
 * @param num  number of items
 * @return Returns mean
 */
inline double _mean(double sum, Uint num)
{
   return num ? sum / num : 0.0;
}

/**
 * Compute mean.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @return Returns mean over [beg .. end)
 */
template<class Iterator> double mean(Iterator beg, Iterator end)
{
   double s = 0.0;
   int n = 0;
   for (; beg != end; ++beg, ++n)
      s += *beg;
   return _mean(s, n);
}

/**
 * Compute variance.
 * @param sum  sum of items.
 * @param sum2 sum of squares of items.
 * @param num  number of items.
 * @return Returns variance
 */
inline double _var(double sum, double sum2, Uint num)
{
   return num > 1 ? (sum2 - sum*sum/num) / (num-1) : 0.0;
}

/**
 * Compute variance.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @return Returns variance over [beg .. end)
 */
template<class Iterator> double var(Iterator beg, Iterator end)
{
   double s = 0.0, s2 = 0.0;
   int n = 0;
   for (; beg != end; ++beg, ++n) {
      s += *beg;
      s2 += *beg * *beg;
   }
   return _var(s, s2, n);
}

/**
 * Compute variance.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @return Returns variance over [beg .. end)
 */
template<class Iterator> double varp(Iterator beg, Iterator end)
{return var(beg, end);}

/**
 * Compute variance, given the mean.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @param mean mean.
 * @return Returns variance over [beg .. end) given the mean.
 */
template<class Iterator> double var(Iterator beg, Iterator end, double mean)
{
   double s = 0.0;
   int n = 0;
   for (; beg != end; ++beg, ++n) {
      double d = *beg - mean;
      s += d * d;
   }
   return n > 1 ? s/(n-1) : 0.0;
}

/**
 * Compute standard deviation.
 * @param sum  sum of items.
 * @param sum2 sum of squares of items.
 * @param num  number of items.
 * @return Returns standard deviation.
 */
inline double _sdev(double sum, double sum2, Uint num)
{
   return sqrt(_var(sum, sum2, num));
}

/**
 * Compute standard deviation.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @return Returns standard deviation over [beg .. end)
 */
template<class Iterator> double sdev(Iterator beg, Iterator end)
{
   return sqrt(var(beg,end));
}

/**
 * Compute standard deviation.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @return Returns standard deviation over [beg .. end)
 */
template<class Iterator> double sdevp(Iterator beg, Iterator end)
{
   return sdev(beg, end);
}

/**
 * Compute standard deviation, given the mean.
 * @param beg  start iterator.
 * @param end  end iterator.
 * @param mean mean.
 * @return Returns standard deviation over [beg .. end) given the mean.
 */
template<class Iterator> double sdev(Iterator beg, Iterator end, double mean)
{
   return sqrt(var(beg, end, mean));
}

/**
 * Return a random number in 0..n-1.
 * @param n upper bound.
 * @return Returns a random number in 0..n-1.
 */
inline Uint rand(Uint n) {return std::rand() % n;}

/**
 * Evaluate the standard normal cumulative density function at a given point.
 * @param v the point at which to evaluate
 * @return P(X < v) according to the standard normal distribution.
 */
double stdNormalCDF(double v);

/**
 * Evaluate the normal cumulative density function at a given point.
 * @param v the point at which to evaluate
 * @param mean the mean of the normal distn
 * @param sigma the standard deviation of the normal distn
 * @return P(X < v) according to the normal distribution.
 */
inline double normalCDF(double v, double mean, double sigma) {
   return stdNormalCDF((v - mean) / sigma);
}

}
#endif
