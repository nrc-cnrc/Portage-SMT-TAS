/**
 * @author Samuel Larkin
 * @file cache_value.h
 * @brief Precomputes values of FUNC in the range [min, max).
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#ifndef __CACHE_VALUE_H__
#define __CACHE_VALUE_H__

#include "portage_defs.h"

namespace Portage {

/// Precomputes values for FUNC(x) in the range x in [min, max).
template <class T, class FUNC>
class cacheValue {
   friend class test_cacheValue;

   private:
      const Uint min;   /// Minimum value for which we hold a value.
      const Uint max;   /// Maximum value for which we hold a value.
      FUNC func;        /// Function we want to precompute.
      vector<T> values; /// Precomputed values.

   public:
      /**
       * Default constructor.
       * @param func  function that we want to precompute.
       * @param min   minimum value we want to precompute.
       * @param max   maximum value we want to precompute.
       */
      cacheValue(FUNC func, Uint max, Uint min = 0)
      : min(min)
      , max(max)
      , func(func)
      {
         values.reserve(max-min);
         for (Uint i(min); i<max; ++i) {
            values.push_back(func(i));
         }
      }

      /** Get the value of FUNC(value) from cache if cached or computes it.
       * @param value  value of x.
       * @return Returns FUNC(x).
       */
      T operator()(Uint value) const {
         return inCache(value) ? values[value-min] : func(value);
      }

   private:
      /**
       * Checks if the value is cached.
       * @param value  value of x is cached.
       * @return Returns true if value is in [min, max)
       */
      bool inCache(Uint value) const {
         return min <= value && value < max;
      }
};

}  // ends namespace Portage

#endif // __CACHE_VALUE_H__
