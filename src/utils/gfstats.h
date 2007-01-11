/**
 * @author George Foster
 * @file gfstats.h Simple stats algorithms.
 * 
 * 
 * COMMENTS: 
 *
 * Template-haters beware.
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#ifndef GFSTATS_H
#define GFSTATS_H

#include <stdlib.h>
#include <utility>
#include <cmath>
#include <map>
#include <vector>
#include "portage_defs.h"

namespace Portage {

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
   return n ? s / n : 0.0;
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
   return n > 1 ? (s2 - s*s/n) / (n-1) : 0.0;
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

}
#endif
