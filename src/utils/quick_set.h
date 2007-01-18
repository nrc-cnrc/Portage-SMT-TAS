/**
 * @author George Foster
 * @file quick_set.h  Sets of integers with constant-time insert (amortized),
 * find, and clear operations
 * 
 * 
 * COMMENTS: 
 *
 * Use this instead of a hash table when you need constant-time clear(), ie
 * when you will be re-using it over and over. Its main drawback is that it
 * requires O(largest-element) space.
 *
 * To use as a map, use the offsets returned by insert() and find(), which are
 * unique and contiguous.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef QUICK_SET_H
#define QUICK_SET_H

#include <vector>
#include <portage_defs.h>

namespace Portage {

/// Hash table with constant-time on clear()
class QuickSet {

   vector<Uint> list;  ///< contains the hash table elements.
   vector<Uint> map;   ///< contains the indexes of the elements in list.

public:

   /// Get the number of elements in the hash table.
   /// @return Returns the number of elements in the hash table.
   Uint size() {return list.size();}

   /**
    * Inserts an element in the hash table.
    * @param elem element to be inserted.
    * @return Returns the index of the inserted elem.
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
    * Finds an element in the hash table.  Return size() if elem not found.
    * @param elem  element to be found.
    * @return Returns the index of elem or size() if not found.
    */
   Uint find(Uint elem) {
      return elem < map.size() && map[elem] < list.size() && list[map[elem]] == elem  ?
	 map[elem] : list.size();
   }

   /// Clears the hash table.
   void clear() {list.resize(0);}

   /// List of set contents.
   /// @return Returns a list containing the elements of the hash table.
   const vector<Uint>& contents() {return list;}

   /// Unit testing function.
   /// @return Returns true if unit testing passes.
   static bool test();
};

  
}
#endif
