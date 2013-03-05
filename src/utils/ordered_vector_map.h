/**
 * @author Eric Joanis
 * @file ordered_vector_map.h A replacement for hash map in particular cases.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 *
 * An extension of stl's vector, with part of the Associative Container
 * interface implemented.  This may be preferable to a hash map when the table
 * won't be very big and space is a concern.  Unlike vector_map, keep the list
 * sorted so find is O(log N).  Note that the find() function returns a normal
 * vector iterator that iterates through ALL elements in the vector following
 * the one found.
 *
 * Comparison of vector_map and ordered_vector_map:
 * vector_map:
 * Insert: O(n). For a new element, the cost is Th(n), whereas if the element
 *         is already present, it depends on where it is in the vector.
 * Lookup: O(n). If not found, Th(n), if found, it depends where the element
 *         is.
 * ordered_vector_map:
 * Insert: O(n). For an element already present, Th(log(n)).  For a new
 *         element, Th(log(n)) for finding where to insert it, plus O(n) for
 *         doing the insertion itself.
 * Lookup: Th(log(n))
 *
 * For a sequence of n inserts, m of which are distinct, the cost per insert is:
 * vector_map:         O(m)
 * ordered_vector_map: O(log(n)) for lookup plus amortized O(m/n*m) for actual
 *                     insertions
 *
 * If ValueT and/or KeyT involve dynamic memory, the cost of doing inserts into
 * ordered_vector_map can be very high, since moving things around ultimately
 * resolves to calling v[i+1] = v[i] for each element being moved.  Thus,
 * ordered_vector_map is probably only appropriate for self-contained KeyT and
 * ValueT types.  However, if ValueT and KeyT are safe to move by memmove,
 * setting UseMemmove = true removes this inefficiency.
 *
 * Notation reminder:
 *  - f(n) = Th(g(n)) = Theta(g(n)) means f(n) is bounded above and below by
 *    g(n), given appropriate constants.
 *  - f(n) = Omega(g(n)) means f(n) is bounded below by g(n), given an
 *    appropriate constant
 *  - f(n) = O(g(n)) means f(n) is bounded above by g(n), given an appropriate
 *    constant.
 */

#ifndef ORDERED_VECTOR_MAP_H
#define ORDERED_VECTOR_MAP_H

#include <vector>
#include <utility>
#include <algorithm> // for lower_bound()
#include <cstring> // for memmove() and memcmp()
#include <cmath> // for ceil()
#include <cstdio> // for fprintf()

using namespace std;

namespace Portage
{
   /**
    * A replacement for hash map in particular cases.  An extension of stl's
    * vector, with part of the Associative Container interface implemented.
    * This may be preferable to a hash map when the table won't be very big and
    * space is a concern.  Unlike vector_map, keep the list sorted so find is
    * O(log N).  Note that the find() function returns a normal vector iterator
    * that iterates through ALL elements in the vector following the one found.
    *
    * The UseMemmove template parameter should be set to true if KeyT and ValueT
    * are safe to move around in memory using memmove, i.e., they don't require
    * the use of assignment operators.  This is the case if ValueT is a
    * std::vector, despite the dynamic memory involved, since the base vector
    * structure is simply three pointers.
    */
   template <class KeyT, class ValueT, bool UseMemmove = false>
   class ordered_vector_map : public vector< pair<KeyT, ValueT> >
   {
      typedef vector< pair<KeyT, ValueT> > basetype;
      typedef ordered_vector_map<KeyT, ValueT, UseMemmove> thistype;
      typedef pair<KeyT, ValueT> StorageT;
      struct KeyLessThan {
         bool operator()(const StorageT& x, const StorageT& y) const {
            return x.first < y.first;
         }
         // needed for std::lower_bound
         bool operator()(const StorageT& x, const KeyT& y) const {
            return x.first < y;
         }
      };
      struct KeyNotLessThan {
         bool operator()(const StorageT& x, const StorageT& y) const {
            return !(x.first < y.first);
         }
      };

   public:
      /// Constructor with safety checking
      ordered_vector_map() {
         if ( UseMemmove )
            assert(test_safe_memmove() &&
                   "Use of ordered_vector_map with UseMemmove with a KeyT or ValueT type that is not safe");
      }

      //@{
      /// Needed to compile with g++ 3.4.3
      using vector< pair<KeyT, ValueT> >::begin;
      using vector< pair<KeyT, ValueT> >::back;
      using vector< pair<KeyT, ValueT> >::end;
      //@}

      typedef typename vector< pair<KeyT, ValueT> >::iterator iterator;
      typedef typename vector< pair<KeyT, ValueT> >::const_iterator const_iterator;

      /**
       * Finds a key.
       * @param key the key to be found
       * @return Returns a iterator that points to the key value or
       *         the end iterator if not found
       */
      const_iterator find(const KeyT &key) const
      {
         const_iterator it =
            std::lower_bound(begin(), end(), key, KeyLessThan());
         if ( it != end() && it->first == key )
            return it;
         else
            return end();
      } // find

      /**
       * Finds a key.
       * @param key the key to be found
       * @return Returns a iterator that points to the key value or
       *         the end iterator if not found
       */
      iterator find(const KeyT &key)
      {
         iterator it =
            std::lower_bound(begin(), end(), key, KeyLessThan());
         if ( it != end() && it->first == key )
            return it;
         else
            return end();
      } // find

      /**
       * Finds a key and returns a reference on its value or adds a key.
       * @param key the key to be found
       * @return Returns a reference on the key's value or add a new key
       */
      ValueT& operator[](const KeyT &key)
      {
         iterator it =
            std::lower_bound(begin(), end(), key, KeyLessThan());
         if ( it == end() || it->first != key ) {
            if ( !UseMemmove ) {
               it = this-> insert(it, make_pair(key, ValueT()));
            } else {
               size_t i = it - begin();
               const size_t old_size = basetype::size();
               if (1 && basetype::size() == basetype::capacity()) {
                  // Here we control the growth of the base vector ourselves,
                  // so that we can move the memory to the new location via
                  // memmove instead of using potentially expensive assignment
                  // operators.  We rely on the user promises about KeyT and
                  // ValueT being movable when they set UseMemmove=true.
                  const double growth_factor = 2;
                  const size_t new_size =
                     old_size == 0 ? 1 : ceil(double(old_size) * growth_factor);
                  thistype new_storage;
                  new_storage.reserve(new_size);
                  if ( old_size > 0 ) {
                     #ifdef _GLIBCXX_VECTOR
                        // faster code: set to the new size without calling
                        // constructors, since we're about to overwrite the
                        // memory.
                        new_storage.basetype::_M_impl._M_finish =
                           new_storage.basetype::_M_impl._M_start + old_size;
                        assert(new_storage.size() == old_size);
                     #else
                        new_storage.resize(old_size);
                     #endif
                     memmove(&(new_storage.basetype::operator[](0)),
                             &(basetype::operator[](0)),
                             old_size * sizeof(StorageT));
                  }
                  #ifdef _GLIBCXX_VECTOR
                     // faster code: set the size to 0 directly, so destructors
                     // won't get called, instead of overwriting each element
                     // with a fresh default-constructed value.
                     basetype::_M_impl._M_finish = basetype::_M_impl._M_start;
                  #else
                     for ( size_t i = 0; i < old_size; ++i )
                        ::new(reinterpret_cast<void*>(&(basetype::operator[](i)))) StorageT();
                  #endif
                  basetype::swap(new_storage);
               }
               basetype::resize(old_size+1);
               if ( old_size > i )
                  memmove(&(basetype::operator[](i+1)),
                          &(basetype::operator[](i)),
                          sizeof(StorageT) * (old_size - i));
               ::new(reinterpret_cast<void*>(&(basetype::operator[](i)))) StorageT();
               basetype::operator[](i).first = key;
               it = begin() + i;
            }
         }
         assert(it->first == key);
         return it->second;
      } // operator[]

      /**
       * Perform multi-set union on two maps, keeping the result in *this
       * @param x  ordered_vector_map to add to *this.
       */
      void operator+=(const thistype& x)
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

      /// For unit testing only
      bool test_is_sorted() const {
         return std::adjacent_find(begin(), end(), KeyNotLessThan()) == end();
      }

      /// For programmers to check that their type is safe with UseMemmove
      /// @param verbose  if true, show the memory differences when a type is not safe
      template <class T> static bool test_safe_memmoveT(bool verbose = false) {
         T x = T();
         T y = T();
         if ( memcmp(&x, &y, sizeof(T)) != 0 ) {
            if ( verbose ) {
               const unsigned char* c_x = reinterpret_cast<const unsigned char*>(&x);
               fprintf(stderr, "\nx: ");
               for ( Uint i = 0; i < sizeof(T); ++i ) {
                  fprintf(stderr, "%02x", c_x[i]);
                  if ( i % 8 == 7) cerr << " ";
               }
               fprintf(stderr, "\ny: ");
               const unsigned char* c_y = reinterpret_cast<const unsigned char*>(&y);
               for ( Uint i = 0; i < sizeof(T); ++i ) {
                  fprintf(stderr, "%02x", c_y[i]);
                  if ( i % 8 == 7) cerr << " ";
               }
               fprintf(stderr, "\n");
            }
            return false;
         }
         return true;
      }
      /// For programmers to check that their type is safe with UseMemmove
      /// @param verbose  if true, show the memory differences when a type is not safe
      static bool test_safe_memmove(bool verbose = false) {
         return test_safe_memmoveT<KeyT>(verbose) && test_safe_memmoveT<ValueT>(verbose);
      }
   }; // ordered_vector_map
} // Portage

#endif // ORDERED_VECTOR_MAP_H

