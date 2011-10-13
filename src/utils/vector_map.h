/**
 * @author Aaron Tikuisis
 * @file vector_map.h A replacement for hash map in particular cases.
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * An extension of stl's vector, with part of the Associative Container
 * interface implemented.  This may be preferable to a hash map when the table
 * won't be very big and space is a concern.  Note that the find() function
 * returns a normal vector iterator that iterates through ALL elements in the
 * vector following the one found.
 */

#ifndef VECTOR_MAP_H
#define VECTOR_MAP_H

#include <vector>
#include <utility>

using namespace std;

namespace Portage
{
   /**
    * A replacement for hash map in particular cases.  An extension of stl's
    * vector, with part of the Associative Container interface implemented.
    * This may be preferable to a hash map when the table won't be very big
    * and space is a concern.  Note that the find() function returns a normal
    * vector iterator that iterates through ALL elements in the vector
    * following the one found.
    */
   template <class KeyT, class DataT>
   class vector_map : public vector< pair<KeyT, DataT> >
   {
   public:
      //@{
      /// Needed to compile with g++ 3.4.3
      using vector< pair<KeyT, DataT> >::begin;
      using vector< pair<KeyT, DataT> >::back;
      using vector< pair<KeyT, DataT> >::end;
      //@}

      typedef typename vector< pair<KeyT, DataT> >::iterator iterator;
      typedef typename vector< pair<KeyT, DataT> >::const_iterator const_iterator;

      /**
       * Finds a key.
       * @param key the key to be found
       * @return Returns a iterator that points to the key value or
       *         the end iterator if not found
       */
      const_iterator find(const KeyT &key) const
      {
         const_iterator it(begin());
         for (; it != end(); ++it)
            if (key == it->first) break;
         return it;
      } // find

      /**
       * Finds a key.
       * @param key the key to be found
       * @return Returns a iterator that points to the key value or
       *         the end iterator if not found
       */
      iterator find(const KeyT &key)
      {
         iterator it(begin());
         for (; it != end(); ++it)
            if (key == it->first) break;
         return it;
      } // find

      /**
       * Finds a key and returns a reference on its value or adds a key.
       * @param key the key to be found
       * @return Returns a reference on the key's value or add a new key
       */
      DataT &operator[](const KeyT &key)
      {
         iterator it = find(key);
         if (it == end()) {
            push_back(make_pair(key, DataT()));
            return back().second;
         } else {
            return it->second;
         }
      } // operator[]

      /**
       * Perform multi-set union on two maps, keeping the result in *this
       * @param x  vector_map to add to *this.
       */
      void operator+=(const vector_map<KeyT, DataT>& x)
      {
         for ( const_iterator it(x.begin()), end(x.end()); it != end; ++it )
            operator[](it->first) += it->second;
      }

      /**
       * Find the maximal element in *this.
       * Returns end() when *this is empty.
       */
      iterator max()
      {
         iterator max_it(begin()), it(begin());
         for (; it != end(); ++it)
            if ( it->second > max_it->second ) max_it = it;
         return max_it;
      }

      /**
       * Find the maximal element in *this.
       * Returns end() when *this is empty.
       */
      const_iterator max() const
      {
         const_iterator max_it(begin()), it(begin());
         for (; it != end(); ++it)
            if ( it->second > max_it->second ) max_it = it;
         return max_it;
      }

   }; // vector_map
} // Portage

#endif // VECTOR_MAP_H

