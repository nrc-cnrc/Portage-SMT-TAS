/**
 * @author Eric Joanis
 * @file lazy_stl.h Wrappers for STL methods that are often called the same way
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technoogies
 * Institut de technologie de l'information / Institute for Information Technoloy
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __LAZY_STL_H__
#define __LAZY_STL_H__

#include "portage_defs.h"
#include <vector>
#include <algorithm>

namespace Portage {

/**
 * extensions to STL algorithms with convenience steps added - don't declare
 * "using namespace lazy;" or you will hide the standard functions these wrap.
 * Instead, it is preferable to call lazy::<function> explicitely.
 */
namespace lazy {
   //@{
   /**
    * Wrapper for std::pop_heap that returns the element popped and actually
    * shortens the heap itself.
    * @param heap  The heap to pop the best element from - will contain one
    *              fewer element after this call.
    * @param cmp   "Less Than" callable entity for T
    * @pre is_heap(heap) and !heap.empty()
    * @return  the elem
    */
   template<typename T, typename T_Compare>
      T pop_heap(vector<T>& heap, T_Compare cmp) {
         assert(!heap.empty());
         std::pop_heap(heap.begin(), heap.end(), cmp);
         T result = heap.back();
         heap.pop_back();
         return result;
      }

   template<typename T>
      T pop_heap(vector<T>& heap) {
         assert(!heap.empty());
         std::pop_heap(heap.begin(), heap.end());
         T result = heap.back();
         heap.pop_back();
         return result;
      }
   //@}

   //@{
   /**
    * Wrapper for std::push_heap that actually adds the item to the heap as
    * well as re-heapifying heap.
    * @param heap  The heap to push a new item to - will contain one more
    *              element after this call.
    * @param item  The item to add to heap
    * @param cmp   "Less Than" callable entity for T
    * @pre is_heap(heap)
    */
   template<typename T, typename T_Compare>
      void push_heap(vector<T>& heap, T item, T_Compare cmp) {
         heap.push_back(item);
         std::push_heap(heap.begin(), heap.end(), cmp);
      }

   template<typename T>
      void push_heap(vector<T>& heap, T item) {
         heap.push_back(item);
         std::push_heap(heap.begin(), heap.end());
      }
   //@}

} // lazy
} // Portage

#endif // __LAZY_STL_H__
