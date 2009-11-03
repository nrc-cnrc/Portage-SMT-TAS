/**
 * @author George Foster
 * @file quick_set.h  Sets of integers with constant-time insert (amortized),
 * find, and clear operations
 *
 * COMMENTS:
 *
 * Use this instead of a hash table when you need constant-time clear(), ie
 * when you will be re-using it over and over. Its main drawback is that it
 * requires O(largest-element) space. Also, there's no delete operation, 
 * though this would not be difficult to add.
 *
 * To use as a map, use the offsets returned by insert() and find(), which are
 * unique and contiguous. That is, to map from integers to some type T, use
 * vector<T> contents(). To find the element associated with index i, use
 * find(i) to index into contents. To insert an element, test first with
 * find(); if it's new, append it to contents.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef QUICK_SET_H
#define QUICK_SET_H

#include <vector>
#include <portage_defs.h>

namespace Portage {

/// Set of integers with constant-time on clear()
class QuickSet {

   /// The elements in the set, in order of insertion.
   vector<Uint> list;

   /// Map for constant-time verification of set membership. For each element e
   /// in the set, map[e] contains the index of e in list, ie list[map[e]] == e.
   /// If this doesn't hold, or if it's undefined due to size violations, then
   /// e is not in the set.
   vector<Uint> map;

public:

   /// Number of elements in the set.
   Uint size() { return list.size(); }

   /// Set is empty.
   bool empty() { return list.empty(); }

   /**
    * Add an element to the set.
    * @param elem element to be inserted.
    * @return index of the inserted elem.
    */
   Uint insert(Uint elem) {
      if (find(elem) == size()) {
         if (elem >= map.size()) map.resize(2*(elem+1));
         map[elem] = list.size();
         list.push_back(elem);
      }
      return map[elem];
   }

   /**
    * Find an element in the set.
    * @param elem element sought.
    * @return index of elem, or size() if not found.
    */
   Uint find(Uint elem) {
      return elem < map.size() && map[elem] < list.size() && list[map[elem]] == elem  ?
         map[elem] : list.size();
   }

   /// Clear the contents.
   void clear() {list.resize(0);}

   /// List of set contents.
   /// @return list containing the integers in the set, in order of insertion.
   const vector<Uint>& contents() {return list;}

   /// Unit testing function.
   /// @return true if unit testing passes.
   static bool test();
};


}
#endif
