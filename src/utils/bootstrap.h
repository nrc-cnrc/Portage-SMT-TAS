/**
 * @author George Foster
 * @file bootstrap.h Bootstrap resampling for confidence intervals.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include <utility>
#include <cmath>
#include <map>
#include <vector>
#include "gfstats.h"

namespace Portage {

/**
 * Compute a confidence interval for a given statistic over a given sample
 * using bootstrap resampling. See bootstrapMeanConfInterval() for an example
 * instantiation for mean.
 *
 * @param beg start of sample, must be a random access iterator
 * @param end end of sample, must be a random access iterator
 * @param stat statistic over sample; a function or functor with signature:
 * double f(Iterator beg, Iterator end). 
 * @param conf probability of confidence interval, in (0,1]
 * @param m number of resampling runs
 * @param seed seed for rand() fcn; if this is 0, seed isn't set
 * @return delta such that P(s in [s' - delta, s' + delta]) >= conf, where s'
 * is the value of stat on the original sample.
 */
template<class Iterator, class Statistic> 
double bootstrapConfInterval(Iterator beg, Iterator end, Statistic stat,
			     double conf = 0.95, Uint m = 1000, Uint seed = 0) 
{
   using namespace std;
   Uint n = end - beg;
   typedef typename std::iterator_traits<Iterator>::value_type T;
   vector<T> resample(n);
   vector<double> deltas(m);
   if (seed != 0) srand(seed);

   double sm = stat(beg, end);
   copy(beg, end, resample.begin());
   for (Uint i = 0; i < m; ++i) {
      for (Uint j = 0; j < n; ++j)
	 beg[j] = resample[rand(n)];
      deltas[i] = abs(sm - stat(beg, end));
   }
   copy(resample.begin(), resample.end(), beg);
   
   sort(deltas.begin(), deltas.end());
   return deltas[(Uint)ceil(conf * m)-1];
}

/**
 * Compute a confidence interval for given statistic(s) over a given sample.
 * This is exactly the same as the previous version, except that the stat
 * functor must return a vector of values rather than a scalar. This is useful
 * for calculating a bunch of statistics over a sample in parallel. The size of
 * the returned vector must match that of res, where the results for each
 * component are put.
 *
 * @param beg  start of sample, must be a random access iterator
 * @param end  end of sample, must be a random access iterator
 * @param stat statistic over sample; a function or functor with signature:
 *             double f(Iterator beg, Iterator end). 
 * @param res  place to put results; size must match size of vector returned by
 *             stat; res[i] is the # of times the ith value in stat was maximum.
 * @param conf probability of confidence interval, in (0,1]
 * @param m    number of resampling runs
 * @param seed seed for rand() fcn; if this is 0, seed isn't set
 */
template<class Iterator, class Statistic> 
void bootstrapConfInterval(Iterator beg, Iterator end,
			   Statistic stat, vector<double>& res,
			   double conf = 0.95, Uint m = 1000,
			   Uint seed = 0)
{
   using namespace std;
   Uint n = end - beg;
   typedef typename std::iterator_traits<Iterator>::value_type T;
   typedef vector<double> Vect;
   vector<T> resample(n);
   vector<Vect> deltas(res.size());
   if (seed != 0) srand(seed);

   Vect sm(stat(beg,end));
   copy(beg, end, resample.begin());
   for (Uint i = 0; i < m; ++i) {
      for (Uint j = 0; j < n; ++j)
	 beg[j] = resample[rand(n)];
      Vect s(stat(beg,end));
      for (Uint k = 0; k < res.size(); ++k)
	 deltas[k].push_back(abs(sm[k]-s[k]));
   }
   copy(resample.begin(), resample.end(), beg);

   for (Uint k = 0; k < res.size(); ++k) {
      sort(deltas[k].begin(), deltas[k].end());
      res[k] = deltas[k][(Uint)ceil(conf * m)-1];
   }
}

/**
 * Compute a confidence interval for given statistic(s) over a given sample.
 * Do a comparison among n statistics over a sample, using bootstrap
 * resampling: record the number of times each statistic attained the maximum
 * value out of the set of all statistics.
 *
 * @param beg start of sample, must be a random access iterator
 * @param end end of sample, must be a random access iterator
 * @param stat set of statistics over sample; a function or functor with signature:
 *        vector<double> f(Iterator beg, Iterator end). 
 * @param res place to put results; size must match size of vector returned by
 *        stat; res[i] is the # of times the ith value in stat was maximum.
 * @param m number of resampling runs
 * @param seed seed for rand() fcn; if this is 0, seed isn't set
 */
template<class Iterator, class Statistic> 
void bootstrapNWiseComparison(Iterator beg, Iterator end,
			      Statistic stat, vector<Uint>& res,
			      Uint m = 1000, Uint seed = 0)
{
   using namespace std;
   Uint n = end - beg;
   typedef typename std::iterator_traits<Iterator>::value_type T;
   typedef vector<double> Vect;
   vector<T> resample(n);
   vector<Vect> deltas(res.size());
   if (seed != 0) srand(seed);
   res.assign(res.size(), 0);

   copy(beg, end, resample.begin());
   for (Uint i = 0; i < m; ++i) {
      for (Uint j = 0; j < n; ++j)
	 beg[j] = resample[rand(n)];
      Vect s(stat(beg,end));
      ++res[max_element(s.begin(), s.end())-s.begin()];
   }
   copy(resample.begin(), resample.end(), beg);
 }

/**
 * Compute a confidence interval for the mean of a given sample using bootstrap
 * resampling.
 *
 * @param beg   start of sample, must be a random access iterator
 * @param end   end of sample, must be a random access iterator
 * @param conf  probability of confidence interval, in (0,1]
 * @param m     number of resampling runs
 * @param seed  seed for rand() fcn; if this is 0, seed isn't set
 * @return delta such that P(s in [s' - delta, s' + delta]) >= conf, where s'
 * is the value of stat on the original sample.
 */
template<class Iterator> 
double bootstrapMeanConfInterval(Iterator beg, Iterator end,
				 double conf = 0.95, Uint m = 1000, 
				 Uint seed = 0)
{
   return bootstrapConfInterval(beg, end, mean<Iterator>, conf, m, seed);
}

}// ends Portage namespace
#endif
