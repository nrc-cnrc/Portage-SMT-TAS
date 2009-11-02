/**
 * @author Eric Joanis
 * @file ordered_map.h Map optimized for multiple distinct inserts followed
 *                     by ordered iteration.
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef __ORDERED_MAP_H__
#define __ORDERED_MAP_H__

#include <vector>
#include <algorithm>
#include "gfmath.h"
#include "binio.h"

namespace Portage
{

// EJJ: Yes, UGLY, this is a macro.  But there is a reason.  It gets called
// billions of times, so I want to inline it, but if I write it as an inline
// method, the compiler refuses, inlining sort_self() into this instead, so I'm
// using a macro.  If you can't live with this, I guess I could cut and paste
// this line everywhere I use it instead.
#define ORDERED_MAP_SORT_SELF if ( sorted_size != storage.size() ) sort_self();

/**
 * Map optimized for multiple distinct inserts followed by ordered iteration,
 * and compact storage.
 *
 * If you need deletion, or frequent alternation between inserts and lookup,
 * use tr1::unordered_map instead.  This data structure really only makes sense
 * when you need many inserts followed by ordered iteration
 *
 * Has sparse vector/matrix semantics: any explicit query for an unitialized
 * element returns 0, whereas iteration loops only over explicitly initialized
 * elements.
 *
 * If all the inserts are done in increasing order of key, this class is
 * optimized so that all methods with complexity O(n log n) have complexity
 * O(n) instead.
 */
template <class KeyT, class ValueT>
class OrderedMap {
   typedef pair<KeyT,ValueT> StorageT;
   struct StorageTLessThan {
      bool operator()(const StorageT& x, const StorageT& y) const {
         return x.first < y.first;
      }
      bool operator()(const StorageT& x, const KeyT& y) const {
         return x.first < y;
      }
   };
   /**
    * Underlying storage
    * Semantics: if more than one element has the same key, the last one in
    * the vector is the current value for that key; size() is the number of
    * elements in storage with distinct keys.
    */
   mutable vector<StorageT> storage;
   /// The part of storage that is already sorted.
   mutable Uint sorted_size;
   /// Whether we're currently in additive or replacement mode
   bool additive_mode;

   /// Sort self and remove duplicates, as necessary, for fast sequential
   /// iteration.
   /// Postcondition: ordered
   /// TODO: optimize to take advantage of previously sorted parts.
   void sort_self() const {
      if ( sorted_size == storage.size() ) return;

      // Optimization: don't re-sort if already inserted in order
      bool needs_sorting(false);
      bool has_duplicates(false);
      const Uint last_sorted(max(0, int(sorted_size)-1));
      const Uint storage_size(storage.size());
      KeyT max_key = storage[last_sorted].first;
      for ( Uint i = last_sorted + 1; i < storage_size; ++i ) {
         if ( storage[i].first < max_key ) {
            needs_sorting = true;
            break;
         } else if ( storage[i].first == max_key ) {
            has_duplicates = true;
         } else {
            max_key = storage[i].first;
         }
      }

      if ( needs_sorting ) {
         if ( additive_mode ) {
            // Use the cheaper std::sort() when we don't need stable sort
            std::sort(storage.begin(), storage.end(), StorageTLessThan());
         } else {
            // Stable sort is required to preserve the original insertion order,
            // since only the last insert counts in the case of duplicates.
            std::stable_sort(storage.begin(), storage.end(), StorageTLessThan());
         }
         // after sorting, we always assume we need to remove duplicates
         has_duplicates = true;
      }
      if ( has_duplicates ) {
         remove_duplicates();
      }
      sorted_size = storage.size();


      /*
      const Uint storage_size(storage.size());
      const double log_storage_size(log2(storage_size));
      if ( (storage_size - sorted_size) < log_storage_size ) {
         //insertion_sort_self();
      } else {
         Uint disorder_count(0);
         KeyT max_key = storage[max(0,int(sorted_size)-1)].first;
         for ( Uint i = sorted_size; i < storage_size; ++i ) {
            if ( storage[i].first < max_key )
               ++disorder_count;
            else
               max_key = storage[i].first;
         }
         if ( disorder_count < log_storage_size ) {
            //insertion_sort_self();
         } else {
            std::stable_sort(storage.begin(), storage.end(), StorageTLessThan());
            remove_duplicates();
            sorted_size = storage.size();
         }
      }
      */
   }

   /// Remove any duplicates after having performed std::stable_sort on self.
   void remove_duplicates() const {
      Uint to = 0;
      const Uint storage_size(storage.size());
      for ( Uint from(1); from < storage_size; ++from ) {
         if ( storage[from].first == storage[to].first ) {
            if ( additive_mode )
               storage[to].second += storage[from].second;
            else
               storage[to].second = storage[from].second;
         } else {
            ++to;
            if ( to < from ) storage[to] = storage[from];
         }
      }
      storage.resize(to+1);
   }

 public:

   /// Default constructor
   /// @param capacity  reserve storage for capapacity elements
   OrderedMap(Uint capacity = 0)
      : sorted_size(0)
      , additive_mode(true)
   { reserve(capacity); }

   /// Swap the contents of this and that
   void swap(OrderedMap& that) {
      storage.swap(that.storage);
      std::swap(sorted_size, that.sorted_size);
      std::swap(additive_mode, that.additive_mode);
   }

   /// Reserve storage
   void reserve(size_t capacity) { storage.reserve(capacity); }

   /// Return whether the map is empty
   /// Complexity: O(1)
   bool empty() const { return storage.empty(); }

   /// Return the number of elements set in the map.
   /// Complexity: O(n log n) if storage is not ordered, O(1) if ordered.
   /// Postcondition: storage is ordered
   size_t size() const { ORDERED_MAP_SORT_SELF; return storage.size(); }

   /// Remove all elements from the map
   /// Complexity: O(1)
   /// Postconditions: storage is ordered; empty(); size() == 0
   void clear() { storage.clear(); sorted_size = 0; }

   /// Write self in binary format
   void writebin(ostream& os) const {
      BinIO::writebin(os, storage);
      BinIO::writebin(os, sorted_size);
      BinIO::writebin(os, additive_mode);
   }
   
   /// Read self in binary format
   void readbin(istream& is) {
      BinIO::readbin(is, storage);
      BinIO::readbin(is, sorted_size);
      BinIO::readbin(is, additive_mode);
   }

   /// Compact the map by removing elements whose value is ValueT() (or 0, for
   /// primitive types),
   void compact() {
      ORDERED_MAP_SORT_SELF;
      Uint to = 0;
      const Uint storage_size(storage.size());
      for ( Uint from(0); from < storage_size; ++from ) {
         if ( storage[from].second != ValueT() ) {
            if ( to < from ) storage[to] = storage[from];
            ++to;
         }
      }
      storage.resize(to);
   }

   /// Insert a key->value pair
   /// If key was previously inserted, replaces the previous value.
   /// Complexity: amortized O(1), or O(n log n) if storage is not ordered and
   /// mode is additive
   /// Postcondition: storage is not ordered and mode is not additive
   void set(KeyT key, ValueT value) {
      if ( additive_mode ) {
         ORDERED_MAP_SORT_SELF;
         additive_mode = false;
      }
      storage.push_back(make_pair(key,value));
   }

   /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
   template <class KeyT1, class KeyT2>
   void set(KeyT1 key1, KeyT2 key2, ValueT value) {
      set(make_pair(key1,key2), value);
   }

   /// Add value to the current value for key, if key was previously inserted,
   /// insert key->value otherwise.
   /// Complexity: amortized O(1), or O(n log n) if storage is not ordered and
   /// mode is not additive.
   /// Postcondition: storage is not ordered and mode is additive
   void add(KeyT key, ValueT value) {
      if ( !additive_mode ) {
         ORDERED_MAP_SORT_SELF;
         additive_mode = true;
      }
      storage.push_back(make_pair(key,value));
   }

   /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
   template <class KeyT1, class KeyT2>
   void add(KeyT1 key1, KeyT2 key2, ValueT value) {
      add(make_pair(key1,key2), value);
   }

   /// Return true iff *this and that have the same key/values pairs,
   /// where sameness is defined as KeyT::operator==() and ValueT::operator==()
   /// returning true.
   bool operator==(const OrderedMap<KeyT, ValueT>& that) const {
      const_iterator this_it(begin()), this_end(end()),
                     that_it(that.begin()), that_end(that.end());
      while ( this_it != this_end && that_it != that_end ) {
         if ( this_it.index() != that_it.index() ) return false;
         if ( *this_it != *that_it ) return false;
         ++this_it;
         ++that_it;
      }
      return this_it == this_end && that_it == that_end;
   }

   /// Return false iff *this and that have the same key/values pairs,
   /// where sameness is defined as KeyT::operator==() and ValueT::operator==()
   /// returning true.
   bool operator!=(const OrderedMap<KeyT, ValueT>& that) const {
      return !operator==(that);
   }

   /// Lookup an element with a sparse data set interpretation.
   /// Complexity: O(n log n) if storage is not ordered, O(log n) if ordered
   /// Postcondition: storage is ordered
   /// @param key key to search for
   /// @return the value associated with key if found, ValueT() otherwise.
   ///         If ValueT is a primitive type, ValueT() == 0 by definition.
   ValueT operator[](KeyT key) const {
      ORDERED_MAP_SORT_SELF;
      typename vector<StorageT>::iterator it =
         std::lower_bound(storage.begin(), storage.end(), key, StorageTLessThan());
      if ( it != storage.end() && it->first == key )
         return it->second;
      else
         return ValueT();
   }

   /// Syntactic sugar: same as []
   ValueT operator()(KeyT key) const { return operator[](key); }

   /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
   template <class KeyT1, class KeyT2>
   ValueT operator()(KeyT1 key1, KeyT2 key2) const {
      return operator[](make_pair(key1,key2));
   }

   /// Iterator over the elements of the map
   class iterator {
      friend class OrderedMap<KeyT, ValueT>;
      typename vector<StorageT>::iterator it;
      iterator(typename vector<StorageT>::iterator it) : it(it) {}
     public:
      iterator() {}
      bool operator==(const iterator& x) const { return it == x.it; }
      bool operator!=(const iterator& x) const { return it != x.it; }
      bool operator<(const iterator& x) const { return it < x.it; }
      bool operator>=(const iterator& x) const { return it >= x.it; }
      KeyT index() const { return it->first; }
      /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
      template <class KeyT1> KeyT1 index1() const { return it->first.first; }
      /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
      template <class KeyT2> KeyT2 index2() const { return it->first.second; }
      KeyT key() const { return it->first; }
      ValueT& operator*() { return it->second; }
      iterator& operator++() { ++it; return *this; }
      iterator operator++(int) { iterator copy = *this; ++it; return copy; }
   };

   /// Begin ordered iteration on the elements of the map
   /// Complexity: O(n log n) if storage is not ordered, O(1) if ordered
   /// Complexity of subsequent ++ operations: O(1) each
   /// Postcondition: storage is ordered
   iterator begin() {
      ORDERED_MAP_SORT_SELF;
      return iterator(storage.begin());
   }

   /// End ordered iteration on the element of the map
   /// Complexity: O(n log n) if storage is not ordered, O(1) if ordered
   /// Postcondition: storage is ordered
   iterator end() {
      ORDERED_MAP_SORT_SELF;
      return iterator(storage.end());
   }

   /// Begin un-ordered iteration over the elements of the map
   /// Complexity: O(1) if storage is ordered or mode is additive
   /// O(n log n) if storage is not ordered and mode is not additive
   /// Semantics: if un-ordered iteration produces the same element twice,
   /// the true value is the sum of the two instances.
   /// Postcondition: mode is additive or storage is ordered
   /// This iterator is invalidated by any call to a method (even a const one)
   /// with Postcondition that storage is ordered.
   iterator begin_unordered() {
      if ( !additive_mode ) { ORDERED_MAP_SORT_SELF; }
      return iterator(storage.begin());
   }

   /// End un-ordered iteration.
   /// Complexity, Semantics and Postcondition are the same as
   /// begin_unordered().
   /// This iterator is invalidated by any call to a method (even a const one)
   /// with Postcondition that storage is ordered.
   iterator end_unordered() {
      if ( !additive_mode ) { ORDERED_MAP_SORT_SELF; }
      return iterator(storage.end());
   }

   /// Find the first position in the ordered map with key >= min_key
   /// Example, to iterate from key = x to key < y, you can use:
   /// for (iterator it(lower_bound(x)), end(lower_bound(y)); it != end; ++it)
   /// Complexity: O(log n) if storage is sorted, O(n log n) otherwise
   /// Postcondition: storage is sorted
   iterator lower_bound(KeyT min_key) {
      ORDERED_MAP_SORT_SELF;
      typename vector<StorageT>::iterator it =
         std::lower_bound(storage.begin(), storage.end(), min_key,
                          StorageTLessThan());
      return iterator(it);
   }

   /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
   template <class KeyT1, class KeyT2>
   iterator lower_bound(KeyT1 min_key1, KeyT2 min_key2) {
      return lower_bound(make_pair(min_key1,min_key2));
   }

   /// Find key in the map.  If key exists, returns an iterator to the element,
   /// which can be modified in place to change the value at key.
   /// If key does not exist, returns end().
   /// Complexity: O(log n) if storage is sorted, O(n log n) otherwise
   /// Postcondition: storage is sorted
   iterator find(KeyT key) {
      ORDERED_MAP_SORT_SELF;
      typename vector<StorageT>::iterator it =
         std::lower_bound(storage.begin(), storage.end(), key,
                          StorageTLessThan());
      if ( it != storage.end() && it->first == key )
         return iterator(it);
      else
         return iterator(storage.end());
   }

   /// Lookup with insertion performed as needed.
   /// If key exists, returns a modifiable iterator to the element.
   /// If key does not exist, insert it and return a modifiable iterator to the
   /// element.
   ///
   /// This method is optimized for use when inserts and lookups are needed
   /// in arbitrary orders, and/or when values need to modified often, or for
   /// a few inserts performed on a map where storage is already ordered.
   ///
   /// Complexity: O(n log n) if storage is not sorted; O(n) if storage is
   /// sorted but key did not previously exist; O(log n) if storage is sorted
   /// and key already exists.
   /// Postcondition: storage is sorted.
   iterator insert_find(KeyT key) {
      ORDERED_MAP_SORT_SELF;
      typename vector<StorageT>::iterator it =
         std::lower_bound(storage.begin(), storage.end(), key,
                          StorageTLessThan());
      if ( it == storage.end() || it->first != key ) {
         it = storage.insert(it, make_pair(key, ValueT()));
         assert(it->first == key);
      }
      return iterator(it);
   }

   /// Const Iterator over the elements of the map
   class const_iterator {
      friend class OrderedMap<KeyT, ValueT>;
      typename vector<StorageT>::const_iterator it;
      const_iterator(typename vector<StorageT>::const_iterator it) : it(it) {}
     public:
      //const_iterator(const iterator& non_const_iter)
      //   : it(non_const_iter.it), is_max_end(false) {}
      bool operator==(const const_iterator& x) const { return it == x.it; }
      bool operator!=(const const_iterator& x) const { return it != x.it; }
      bool operator<(const const_iterator& x) const { return it < x.it; }
      bool operator>=(const const_iterator& x) const { return it >= x.it; }
      KeyT index() const { return it->first; }
      /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
      template <class KeyT1> KeyT1 index1() const { return it->first.first; }
      /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
      template <class KeyT2> KeyT2 index2() const { return it->first.second; }
      KeyT key() const { return it->first; }
      const ValueT& operator*() const { return it->second; }
      const_iterator& operator++() { ++it; return *this; }
      const_iterator operator++(int) { const_iterator copy = *this; ++it; return copy; }
   };

   /// Begin ordered iteration on the elements of the map
   /// Complexity: O(n log n) if storage is not ordered, O(1) if ordered
   /// Complexity of subsequent ++ operations: worst case O(1) each
   /// Postcondition: storage is ordered
   const_iterator begin() const {
      ORDERED_MAP_SORT_SELF;
      return const_iterator(storage.begin());
   }

   /// End ordered iteration on the element of the map
   /// Complexity: O(n log n) if storage is not ordered, O(1) if ordered
   /// Postcondition: storage is ordered
   const_iterator end() const {
      ORDERED_MAP_SORT_SELF;
      return const_iterator(storage.end());
   }

   /// Begin un-ordered iteration over the elements of the map
   /// Complexity: O(1) if storage is ordered or mode is additive
   /// O(n log n) if storage is not ordered and mode is not additive
   /// Semantics: if un-ordered iteration produces the same element twice,
   /// the true value is the sum of the two instances.
   /// Postcondition: mode is additive or storage is ordered
   /// This iterator is invalidated by any call to a method (even a const one)
   /// with Postcondition that storage is ordered.
   const_iterator begin_unordered() const {
      if ( !additive_mode ) { ORDERED_MAP_SORT_SELF; }
      return const_iterator(storage.begin());
   }

   /// End un-ordered iteration.
   /// Complexity, Semantics and Postcondition are the same as
   /// begin_unordered().
   /// This iterator is invalidated by any call to a method (even a const one)
   /// with Postcondition that storage is ordered.
   const_iterator end_unordered() const {
      if ( !additive_mode ) { ORDERED_MAP_SORT_SELF; }
      return const_iterator(storage.end());
   }

   /// Find the first position in the ordered map with key >= min_key
   /// Example, to iterate from key = x to key < y, you can use:
   /// for (const_iterator it(lower_bound(x)), end(lower_bound(y));
   ///      it != end; ++it)
   /// Complexity: O(log n) if storage is sorted, O(n log n) otherwise
   /// Postcondition: storage is sorted
   const_iterator lower_bound(KeyT min_key) const {
      ORDERED_MAP_SORT_SELF;
      typename vector<StorageT>::const_iterator it =
         std::lower_bound(storage.begin(), storage.end(), min_key,
                          StorageTLessThan());
      return const_iterator(it);
   }

   /// Syntactic sugar for when KeyT = pair<KeyT1,KeyT2>
   template <class KeyT1, class KeyT2>
   const_iterator lower_bound(KeyT1 min_key1, KeyT2 min_key2) const {
      return lower_bound(make_pair(min_key1,min_key2));
   }

   /// const_iterator where index() returns the index of the second dimension,
   /// not the full pair<KeyT1,KeyT2>. Only usable when KeyT=pair<KeyT1,KeyT2>.
   template <class KeyT2>
   class const_iterator2 : public const_iterator {
     public:
      KeyT2 index() const { return const_iterator::index().second; }
      const_iterator2(const const_iterator& init) : const_iterator(init) {}
   };

};

} // Portage

#endif // __ORDERED_MAP_H__
